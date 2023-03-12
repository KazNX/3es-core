#include "StreamThread.h"

#include "KeyframeStore.h"

#include <3esview/ThirdEyeScene.h>

#include <3escore/CollatedPacketDecoder.h>
#include <3escore/Log.h>
#include <3escore/PacketReader.h>
#include <3escore/PacketStreamReader.h>

#include <cinttypes>
#include <fstream>

namespace tes::view::data
{
StreamThread::StreamThread(std::shared_ptr<ThirdEyeScene> tes, std::shared_ptr<std::istream> stream)
  : _stream_reader(std::make_unique<PacketStreamReader>(std::exchange(stream, nullptr)))
  , _tes(std::exchange(tes, nullptr))
{
  _keyframes.store = std::make_unique<KeyframeStore>();
  _thread = std::thread([this] { run(); });
}


StreamThread::~StreamThread() = default;


bool StreamThread::isLiveStream() const
{
  return false;
}


void StreamThread::setTargetFrame(FrameNumber frame)
{
  {
    std::scoped_lock guard(_data_mutex);
    _target_frame = frame;
  }
  // Ensure the thread wakes up to step the frame.
  // Note we have unlocked the mutex before the notify call.
  _notify.notify_all();
}


FrameNumber StreamThread::targetFrame() const
{
  std::scoped_lock guard(_data_mutex);
  return _target_frame.has_value() ? *_target_frame : 0;
}


void StreamThread::setLooping(bool loop)
{
  std::scoped_lock guard(_data_mutex);
  _looping = loop;
}


bool StreamThread::looping() const
{
  std::scoped_lock guard(_data_mutex);
  return _looping;
}


void StreamThread::setPlaybackSpeed(float speed)
{
  std::scoped_lock guard(_data_mutex);
  _playback_speed = std::max(0.01f, speed);
}


float StreamThread::playbackSpeed() const
{
  std::scoped_lock guard(_data_mutex);
  return _playback_speed;
}


void StreamThread::pause()
{
  _paused = true;
}


void StreamThread::unpause()
{
  _paused = false;
  _notify.notify_all();
}


void StreamThread::setAllowKeyframes(bool allowed)
{
  std::scoped_lock guard(_data_mutex);
  _keyframes.enabled = allowed;
}


bool StreamThread::allowKeyframes() const
{
  std::scoped_lock guard(_data_mutex);
  return _keyframes.enabled;
}


void StreamThread::setKeyframeSizeInterval(size_t interval_mib)
{
  std::scoped_lock guard(_data_mutex);
  _keyframes.size_interval_mib = interval_mib;
}


size_t StreamThread::keyframeSizeIntervalMiB() const
{
  std::scoped_lock guard(_data_mutex);
  return _keyframes.size_interval_mib;
}


void StreamThread::setKeyframeInterval(FrameNumber interval)
{
  std::scoped_lock guard(_data_mutex);
  _keyframes.frame_interval = interval;
}


FrameNumber StreamThread::keyframeInterval() const
{
  std::scoped_lock guard(_data_mutex);
  return _keyframes.frame_interval;
}


void StreamThread::join()
{
  _quitFlag = true;
  unpause();
  _thread.join();
}


bool StreamThread::checkCompatibility(const PacketHeader *header)
{
  return checkCompatibility(PacketReader(header));
}


bool StreamThread::checkCompatibility(const PacketReader &reader)
{
  const auto version_major = reader.versionMajor();
  const auto version_minor = reader.versionMinor();

  // Exact version match.
  if (version_major == kPacketVersionMajor && version_minor == kPacketCompatibilityVersionMinor)
  {
    return true;
  }
  // Check major version is in the allowed range (open interval).
  if (kPacketCompatibilityVersionMajor < version_major && version_major < kPacketVersionMajor)
  {
    // Major version is between the allowed range.
    return true;
  }

  // Major version match, ensure minor version is in range.
  if (version_major == kPacketVersionMajor && version_minor <= kPacketVersionMinor)
  {
    return true;
  }

  // Major version compatibility match, ensure minor version is in range.
  if (version_major == kPacketCompatibilityVersionMajor &&
      version_minor >= kPacketCompatibilityVersionMinor)
  {
    return true;
  }

  return false;
}


void StreamThread::run()
{
  Clock::time_point next_frame_start = Clock::now();
  // Last position in the stream we can seek to.
  std::istream::pos_type last_seekable_position = 0;
  std::istream::pos_type last_keyframe_position = 0;
  bool at_frame_boundary = false;
  CollatedPacketDecoder packet_decoder;

  _have_server_info = false;
  while (!_quitFlag)
  {
    // Before anything else, check for the target frame being set. This affects catchup and
    // can trigger updates even when paused.
    FrameNumber target_frame = 0;
    switch (checkTargetFrameState(target_frame))
    {
    case TargetFrameState::NotSet:  // Nothing special to do
    default:
      _catchingUp = false;
      std::this_thread::sleep_until(next_frame_start);
      break;
    case TargetFrameState::Behind:  // Go back.
    case TargetFrameState::Ahead:   // Catch up.
      // Reset and seek back.
      if (!_catchingUp)
      {
        skipToClosestKeyframe(target_frame);
        _catchingUp = true;
        // Check again.
        continue;
      }
      break;
    case TargetFrameState::Reached:  // Result normal playback.
      _catchingUp = false;
      next_frame_start = Clock::now();
      break;
    }

    if (blockOnPause())
    {
      continue;
    }

    if (_stream_reader->isEof())
    {
      if (_looping)
      {
        setTargetFrame(0);
        _have_server_info = false;
      }
    }

    at_frame_boundary = false;  // Tracks when we reach a frame boundary.
    while (!_quitFlag && !at_frame_boundary && _stream_reader->isOk() && !_stream_reader->isEof())
    {
      const auto [packet_header, status, stream_position] = _stream_reader->extractPacket();
      if (packet_header)
      {
        // Check the initial packet compability.
        if (!checkCompatibility(packet_header))
        {
          const PacketReader packet(packet_header);
          log::warn("Unsupported packet version: ", packet.versionMajor(), ".",
                    packet.versionMinor());
          continue;
        }

        const auto process_result = processPacket(packet_header, packet_decoder);
        if (process_result.status == ProcessPacketStatus::EndFrame)
        {
          const auto frame_number = _currentFrame.load();
          if (keyframeNeeded(frame_number, stream_position))
          {
            makeKeyframe(frame_number, stream_position);
          }
          next_frame_start = Clock::now();
          at_frame_boundary = true;
        }

        if (process_result.reset_timeline)
        {
          next_frame_start = Clock::now();
        }
        next_frame_start += process_result.frame_interval;
      }
    }
  }
}


void StreamThread::skipBack(FrameNumber targetFrame)
{
  // Simple implementation until we get keyframes.
  setTargetFrame(targetFrame);
}


bool StreamThread::blockOnPause()
{
  if (_paused && targetFrame() == 0)
  {
    std::unique_lock lock(_data_mutex);
    // Wait for unpause.
    _notify.wait(lock, [this] {
      // Note: we can check _target_frame directly since the _data_mutex is locked while checking
      // the condition variable.
      if (!_paused || _target_frame.has_value())
      {
        return true;
      }
      return false;
    });
    return true;
  }
  return false;
}


StreamThread::ProcessPacketResult StreamThread::processPacket(const PacketHeader *packet_header,
                                                              CollatedPacketDecoder &packet_decoder,
                                                              ProcessPacketFlag flags)
{
  ProcessPacketResult result = {};
  result.status = ProcessPacketStatus::Error;
  // Handle collated packets by wrapping the header.
  // This is fine for normal packets too.
  packet_decoder.setPacket(packet_header);

  // Iterate packets while we decode. These do not need to be released.
  while (auto *header = packet_decoder.next())
  {
    PacketReader packet(header);

    // Check extracted packet compability. This may be the same as the one checked above, or
    // it may be a new packet from a CollatedPacket
    if (!checkCompatibility(packet))
    {
      log::warn("Unsupported packet version (extracted): ", packet.versionMajor(), ".",
                packet.versionMinor());
      continue;
    }

    result.status = ProcessPacketStatus::MidFrame;

    // Lock for frame control messages as these tell us to advance the frame and how long to
    // wait.
    switch (packet.routingId())
    {
    case MtControl:
      result.frame_interval += processControlMessage(
        packet, (flags & ProcessPacketFlag::NoFrameEnd) != ProcessPacketFlag::None);
      if (packet.messageId() == CIdFrame)
      {
        result.status = ProcessPacketStatus::EndFrame;
      }
      break;
    case MtServerInfo:
      if (processServerInfo(packet, _server_info))
      {
        result.reset_timeline = !_have_server_info;
        _have_server_info = true;
        _tes->updateServerInfo(_server_info);
      }
      break;
    default:
      _tes->processMessage(packet);
      break;
    }
  }

  return result;
}


StreamThread::Clock::duration StreamThread::processControlMessage(PacketReader &packet,
                                                                  bool ignore_fame_change)
{
  ControlMessage msg;
  if (!msg.read(packet))
  {
    log::error("Failed to decode control packet: ", packet.messageId());
    return {};
  }
  switch (packet.messageId())
  {
  case CIdNull:
    break;
  case CIdFrame: {
    // Frame ending.
    // Work out how long to the next frame.
    const auto dt = (msg.value32) ? msg.value32 : _server_info.default_frame_time;
    if (!ignore_fame_change)
    {
      _tes->updateToFrame(++_currentFrame);
    }
    return std::chrono::microseconds(
      static_cast<uint64_t>(_server_info.time_unit * dt / _playback_speed));
  }
  case CIdCoordinateFrame:
    if (msg.value32 < CFCount)
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
    if (!ignore_fame_change)
    {
      _total_frames = msg.value32;
    }
    break;
  case CIdForceFrameFlush:
    if (!ignore_fame_change)
    {
      _tes->updateToFrame(_currentFrame);
    }
    return std::chrono::microseconds(static_cast<uint64_t>(
      _server_info.time_unit * _server_info.default_frame_time / _playback_speed));
  case CIdReset:
    if (!ignore_fame_change)
    {
      // This doesn't seem right any more. Need to check what the Unity viewer did with this. It may
      // be an artifact of the main thread needing to do so much work in Unity.
      _currentFrame = msg.value32;
      _tes->reset();
    }
    break;
  case CIdKeyframe:
    // NYI
    log::warn("Keyframe control message handling not implemented.");
    break;
  case CIdEnd:
    log::warn("End control message handling not implemented.");
    break;
  default:
    log::error("Unknown control message id: ", packet.messageId());
    break;
  }
  return {};
}


StreamThread::TargetFrameState StreamThread::checkTargetFrameState(FrameNumber &target_frame)
{
  // Mutex lock to check teh
  std::scoped_lock guard(_data_mutex);

  if (!_target_frame.has_value())
  {
    target_frame = 0;
    return TargetFrameState::NotSet;
  }

  target_frame = *_target_frame;
  const auto current_frame = _currentFrame.load();

  if (target_frame < current_frame)
  {
    return TargetFrameState::Behind;
  }

  if (target_frame > current_frame)
  {
    return TargetFrameState::Ahead;
  }

  _target_frame.reset();
  return TargetFrameState::Reached;
}


bool StreamThread::keyframeNeeded(FrameNumber frame_number,
                                  std::istream::pos_type stream_position) const
{
  // const size_t bytes_interval = _keyframes.size_interval_mib * 1024ull * 1024ull;
  const size_t bytes_interval = 0;
  const auto last_keyframe = _keyframes.store->last();

  // Check size and frame intervals have elapsed.
  if (_keyframes.enabled &&
      frame_number >= last_keyframe.frame_number + _keyframes.frame_interval &&
      static_cast<size_t>(stream_position - last_keyframe.position) >= bytes_interval)
  {
    // Time for a keyframe.
    return true;
  }

  // Not enough keyframes or data passed since the last keyframe.
  return false;
}


bool StreamThread::makeKeyframe(FrameNumber frame_number, std::istream::pos_type stream_position)
{
  const auto keyframe_file = std::filesystem::temp_directory_path() /
                             (std::string("3es_keyframe_") + std::to_string(frame_number));
  // saveSnapshot() blocks until it can be serviced in a thread safe manner.
  const auto [ok, saved_frame] = _tes->saveSnapshot(keyframe_file.string());
  if (ok)
  {
    log::info("Make keyframe ", frame_number, " at stream pos ", stream_position);
    _keyframes.store->add({ saved_frame, stream_position, keyframe_file });
  }

  return ok;
}


void StreamThread::skipToClosestKeyframe(FrameNumber target_frame)
{
  // Find the closest keyframe before target_frame.
  KeyframeStore::Keyframe keyframe = {};
  if (_keyframes.store->lookupNearest(target_frame, keyframe))
  {
    if (keyframe.frame_number <= _currentFrame && target_frame >= _currentFrame)
    {
      // Keyframe is in the past. Ignore it.
      return;
    }

    _tes->reset();
    _currentFrame = keyframe.frame_number;
    if (loadSnapshot(keyframe.snapshot_path))
    {
      // Success. Set steam position.
      log::info("Restore keyframe snapshot for target frame ", target_frame, " to frame ",
                keyframe.frame_number, " at stream pos ", keyframe.position);
      _stream_reader->seek(keyframe.position);

      // Make sure we flag the end of the snapshot frame event.
      _tes->updateToFrame(keyframe.frame_number);
      return;
    }

    // Delete the keyframe.
    _keyframes.store->remove(keyframe.frame_number);
  }

  if (target_frame < _currentFrame.load())
  {
    // No keyframe available or failed to load. Skip to stream start.
    _tes->reset();
    _stream_reader->seek(0);
    _currentFrame = 0;
  }
}


bool StreamThread::loadSnapshot(const std::filesystem::path &snapshot_path)
{
  auto stream = std::make_shared<std::ifstream>(snapshot_path.string().c_str());
  if (!stream->is_open())
  {
    return false;
  }

  bool ok = true;
  PacketStreamReader reader(stream);
  CollatedPacketDecoder packet_decoder;

  while (reader.isOk() && !reader.isEof() && ok)
  {
    auto [packet_header, status, stream_pos] = reader.extractPacket();
    if (!packet_header)
    {
      ok = false;
      log::warn("Failed to load snapshot packet.");
      continue;
    }

    // Check the initial packet compatibility.
    if (!checkCompatibility(packet_header))
    {
      ok = false;
      const PacketReader packet(packet_header);
      log::warn("Unsupported packet version: ", packet.versionMajor(), ".", packet.versionMinor());
      continue;
    }

    const auto processing_result =
      processPacket(packet_header, packet_decoder, ProcessPacketFlag::NoFrameEnd);
    if (processing_result.status == ProcessPacketStatus::Error)
    {
      ok = false;
    }
  }

  return ok;
}
}  // namespace tes::view::data
