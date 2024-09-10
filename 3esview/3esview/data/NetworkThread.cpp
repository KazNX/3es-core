#include "NetworkThread.h"

#include "StreamRecorder.h"

#include <3esview/ThirdEyeScene.h>
#include <3esview/camera/Camera.h>
#include <3esview/handler/Camera.h>

#include <3escore/CollatedPacket.h>
#include <3escore/CollatedPacketDecoder.h>
#include <3escore/CoreUtil.h>
#include <3escore/FileConnection.h>
#include <3escore/Log.h>
#include <3escore/PacketBuffer.h>
#include <3escore/PacketReader.h>
#include <3escore/PacketStreamReader.h>
#include <3escore/TcpSocket.h>

#include <cinttypes>
#include <vector>

namespace tes::view::data
{
NetworkThread::NetworkThread(std::shared_ptr<ThirdEyeScene> tes, std::string host, uint16_t port,
                             bool allow_reconnect)
  : _allow_reconnect(allow_reconnect)
  , _host(std::move(host))
  , _port(port)
{
  _tes = std::exchange(tes, nullptr);
  _thread = std::thread([this] { run(); });
}


NetworkThread::~NetworkThread() = default;


bool NetworkThread::isLiveStream() const
{
  return true;
}


void NetworkThread::setTargetFrame(FrameNumber frame)
{
  // Not supported.
  (void)frame;
}


FrameNumber NetworkThread::targetFrame() const
{
  return 0;
}


void NetworkThread::pause()
{
  // Not supported.
}


void NetworkThread::unpause()
{
  // Not supported.
}


void NetworkThread::join()
{
  _quit_flag = true;
  _allow_reconnect = false;
  _thread.join();
}


bool NetworkThread::isRecording() const
{
  return _record != nullptr;
}


std::filesystem::path NetworkThread::recodingPath() const
{
  const std::lock_guard guard(_data_mutex);
  return (isRecording()) ? _record->path() : std::filesystem::path();
}


bool NetworkThread::startRecording(const std::filesystem::path &path)
{
  const std::lock_guard guard(_data_mutex);
  // TODO(KS): this will start mid frame. It may be better to wait to the next frame flush.
  // Conversely, this could be the right behaviour for an unresponsive, or large data connection.
  recordStopUnguarded();
  _record = std::make_unique<StreamRecorder>(path, _server_info);
  if (_record->isOpen())
  {
    return true;
  }
  _record = nullptr;
  return false;
}


bool NetworkThread::endRecording()
{
  const std::lock_guard guard(_data_mutex);
  // TODO(KS): see TODO in startRecording() regarding stopping mid frame.
  return recordStopUnguarded();
}


void NetworkThread::run()
{
  using namespace std::chrono_literals;
  auto socket = std::make_unique<TcpSocket>();
  constexpr auto kReconnectDelay = 200ms;

  do
  {
    const bool connected = socket->open(_host.c_str(), _port);
    _connected = connected;
    _connection_attempted = true;
    if (!connected)
    {
      if (_allow_reconnect)
      {
        std::this_thread::sleep_for(kReconnectDelay);
      }
      continue;
    }

    // Connected.
    configureSocket(*socket);
    runWith(*socket);
    socket->close();
  } while (_allow_reconnect || !_quit_flag);
}


void NetworkThread::configureSocket(TcpSocket &socket)
{
  socket.setNoDelay(true);
  socket.setReadTimeout(0);
  socket.setWriteTimeout(0);
  socket.setReadBufferSize(1024 * 1024);
  socket.setSendBufferSize(4 * 1024);
}


void NetworkThread::runWith(TcpSocket &socket)
{
  CollatedPacketDecoder packet_decoder;
  bool have_server_info = false;
  // We have two buffers here -> redundant.
  // TODO(KS): change the PacketBuffer interface so we can read directly into it's buffer.
  PacketBuffer packet_buffer;
  std::vector<uint8_t> read_buffer(2048u);

  _current_frame = 0;
  _total_frames = 0;

  // Make sure we reset from any previous connection.
  _tes->reset([this] { return stopping(); });

  while (socket.isConnected() && !_quit_flag)
  {
    auto bytes_read = socket.readAvailable(read_buffer.data(), int(read_buffer.size()));
    if (bytes_read <= 0)
    {
      continue;
    }

    packet_buffer.addBytes(read_buffer.data(), int_cast<size_t>(bytes_read));

    if (const auto *packet_header = packet_buffer.extractPacket(read_buffer))
    {
      packet_decoder.setPacket(packet_header);

      while ((packet_header = packet_decoder.next()))
      {
        PacketReader packet(packet_header);
        // Lock for frame control messages as these tell us to advance the frame and how long to
        // wait.
        switch (packet.routingId())
        {
        case MtControl:
          processControlMessage(packet);
          break;
        case MtServerInfo:
          recordPacket(packet);
          if (processServerInfo(packet, _server_info))
          {
            _tes->updateServerInfo(_server_info);
          }
          if (!have_server_info)
          {
            have_server_info = true;
          }
          break;
        default:
          recordPacket(packet);
          _tes->processMessage(packet);
          break;
        }
      }
    }
  }
}

void NetworkThread::processControlMessage(PacketReader &packet)
{
  ControlMessage msg = {};
  if (!msg.read(packet))
  {
    log::error("Failed to decode control packet: ", packet.messageId());
    return;
  }

  // End frame messsages need special handling as that represents a frame flush.
  // We also need to get the recorded camera position from that action.
  if (packet.messageId() != CIdFrame)
  {
    recordPacket(packet);
  }

  switch (packet.messageId())
  {
  case CIdNull:
    break;
  case CIdFrame: {
    const auto current_frame = ++_current_frame;
    camera::Camera camera = {};
    // Frame ending.
    _tes->updateToFrame(current_frame, camera);
    _total_frames = std::max(current_frame, _total_frames);
    const auto elapsed_sever_time = (msg.value32) ? msg.value32 : _server_info.default_frame_time;
    const auto elapsed_micro_seconds = _server_info.time_unit * elapsed_sever_time;
    const auto dt = static_cast<float>(static_cast<double>(elapsed_micro_seconds) * 1e-6);
    recordFlush(dt, camera);
    break;
  }
  case CIdCoordinateFrame:
    if (msg.value32 < static_cast<int32_t>(CoordinateFrame::Count))
    {
      _server_info.coordinate_frame = CoordinateFrame(msg.value32);
      _tes->updateServerInfo(_server_info);
    }
    else
    {
      log::error("Invalid coordinate frame value: ", msg.value32);
    }
    break;
  case CIdFrameCount:
    _total_frames = msg.value32;
    break;
  case CIdForceFrameFlush:
    _tes->updateToFrame(_current_frame);
    break;
  case CIdReset:
    // This doesn't seem right any more. Need to check what the Unity viewer did with this. It may
    // be an artifact of the main thread needing to do so much work in Unity.
    _current_frame = msg.value32;
    _tes->reset([this] { return stopping(); });
    break;
  case CIdKeyframe:
    [[fallthrough]];
  case CIdEnd:
    break;
  default:
    log::error("Unknown control message id: ", packet.messageId());
    break;
  }
}


void NetworkThread::recordPacket(const PacketReader &packet)
{
  const std::lock_guard guard(_data_mutex);
  if (_record && _record->status() == StreamRecorder::State::Recording)
  {
    _record->recordPacket(packet);
  }
}


void NetworkThread::recordFlush(float dt, const camera::Camera &camera)
{
  const std::lock_guard guard(_data_mutex);
  if (_record)
  {
    if (_record->status() == StreamRecorder::State::Recording)
    {
      _record->recordCamera(camera);
      _record->flush(dt);
    }
    else if (_record->status() == StreamRecorder::State::PendingSnapshot)
    {
      // Make sure we fail the snapshot if a quit is requested to prevent a deadlock.
      const auto [success, _] = _tes->saveSnapshot(
        _record->connection(), [this]() { return static_cast<bool>(_quit_flag); });
      if (success)
      {
        _record->markSnapshot();
        log::trace("Recording snapshot taken.");
      }
      else
      {
        recordStopUnguarded();
        log::error("Failed to start recording snapshot.");
      }
    }
  }
}


bool NetworkThread::recordStopUnguarded()
{
  if (!_record)
  {
    return false;
  }

  _record->close();
  _record = nullptr;
  return true;
}
}  // namespace tes::view::data
