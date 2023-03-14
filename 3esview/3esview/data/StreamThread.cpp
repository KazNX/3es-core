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
    const std::scoped_lock guard(_data_mutex);
    _frame.pending_target = frame;
  }
  // Ensure the thread wakes up to step the frame.
  // Note we have unlocked the mutex before the notify call.
  _notify.notify_all();
}


FrameNumber StreamThread::targetFrame() const
{
  const std::scoped_lock guard(_data_mutex);
  return _frame.target.has_value() ? *_frame.target : 0;
}


void StreamThread::setLooping(bool loop)
{
  const std::scoped_lock guard(_data_mutex);
  _looping = loop;
}


bool StreamThread::looping() const
{
  const std::scoped_lock guard(_data_mutex);
  return _looping;
}


void StreamThread::setPlaybackSpeed(float speed)
{
  const std::scoped_lock guard(_data_mutex);
  _playback_speed = std::max(0.01f, speed);
}


float StreamThread::playbackSpeed() const
{
  const std::scoped_lock guard(_data_mutex);
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
  const std::scoped_lock guard(_data_mutex);
  _keyframes.enabled = allowed;
}


bool StreamThread::allowKeyframes() const
{
  const std::scoped_lock guard(_data_mutex);
  return _keyframes.enabled;
}


void StreamThread::setKeyframeSizeInterval(size_t interval_mib)
{
  const std::scoped_lock guard(_data_mutex);
  _keyframes.size_interval_mib = interval_mib;
}


size_t StreamThread::keyframeSizeIntervalMiB() const
{
  const std::scoped_lock guard(_data_mutex);
  return _keyframes.size_interval_mib;
}


void StreamThread::setKeyframeInterval(FrameNumber interval)
{
  const std::scoped_lock guard(_data_mutex);
  _keyframes.frame_interval = interval;
}


FrameNumber StreamThread::keyframeInterval() const
{
  const std::scoped_lock guard(_data_mutex);
  return _keyframes.frame_interval;
}


void StreamThread::join()
{
  _quit_flag = true;
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
  bool at_frame_boundary = false;
  CollatedPacketDecoder packet_decoder;

  _have_server_info = false;
  while (!_quit_flag)
  {
    // Before anything else, check for the target frame being set. This affects catchup and
    // can trigger updates even when paused.
    FrameNumber target_frame = 0;
    switch (checkTargetFrameState(target_frame))
    {
    case TargetFrameState::NotSet:  // No special frame handling to do...
      // ... but we need to handle looping here so as not to try loop when doing catchup operations.
      if (_stream_reader->isEof() && _looping && !_paused)
      {
        setTargetFrame(0);
        _have_server_info = false;
        _frame.catching_up = false;
      }
    default:
      _frame.catching_up = false;
      std::this_thread::sleep_until(next_frame_start);
      break;
    case TargetFrameState::KeyframeSkip: {
      // Try restore a keyframe.
      skipToClosestKeyframe(target_frame);
      continue;
    }
    case TargetFrameState::Behind:  // Go back.
    case TargetFrameState::Ahead:   // Catch up.
      _frame.catching_up = false;
      break;
    case TargetFrameState::Reached:  // Result normal playback.
      _frame.catching_up = false;
      next_frame_start = Clock::now();
      break;
    }

    if (blockOnPause())
    {
      continue;
    }

    at_frame_boundary = false;  // Tracks when we reach a frame boundary.
    while (!_quit_flag && !at_frame_boundary && _stream_reader->isOk() && !_stream_reader->isEof())
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
        if (process_result.status == ProcessPacketStatus::EndFrame ||
            process_result.status == ProcessPacketStatus::EndFrameNaked)
        {
          if (process_result.status == ProcessPacketStatus::EndFrameNaked)
          {
            // Try for a keyframe if needed and possible. To be possible, the current packet_header
            // is a naked packet; it's not collated and/or compressed.
            const auto frame_number = _frame.current.load();
            if (keyframeNeeded(frame_number, stream_position))
            {
              makeKeyframe(frame_number, stream_position);
            }
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


void StreamThread::skipBack(FrameNumber target_frame)
{
  // Simple implementation until we get keyframes.
  setTargetFrame(target_frame);
}


bool StreamThread::blockOnPause()
{
  if (_paused && targetFrame() == 0)
  {
    std::unique_lock lock(_data_mutex);
    // Wait for unpause.
    _notify.wait(lock, [this] {
      // Note: we can check _frame.target directly since the _data_mutex is locked while checking
      // the condition variable.
      if (!_paused || _frame.pending_target.has_value() || _frame.target.has_value())
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
  while (const auto *header = packet_decoder.next())
  {
    PacketReader packet(header);
    const bool naked_packet = packet_header == header;

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
        result.status =
          (naked_packet) ? ProcessPacketStatus::EndFrameNaked : ProcessPacketStatus::EndFrame;
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
                                                                  bool ignore_frame_change)
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
    if (!ignore_frame_change)
    {
      _tes->updateToFrame(++_frame.current);
    }
    return std::chrono::microseconds(
      static_cast<uint64_t>(_server_info.time_unit * dt / static_cast<double>(_playback_speed)));
  }
  case CIdCoordinateFrame:
    if (msg.value32 < CFCount)
    {
      _server_info.coordinate_frame = static_cast<CoordinateFrame>(msg.value32);
      _tes->updateServerInfo(_server_info);
    }
    else
    {
      log::error("Invalid coordinate frame value: ", msg.value32);
    }
    break;
  case CIdFrameCount:
    if (!ignore_frame_change)
    {
      _frame.total = msg.value32;
    }
    break;
  case CIdForceFrameFlush:
    if (!ignore_frame_change)
    {
      _tes->updateToFrame(_frame.current);
    }
    return std::chrono::microseconds(
      static_cast<uint64_t>(_server_info.time_unit * _server_info.default_frame_time /
                            static_cast<double>(_playback_speed)));
  case CIdReset:
    if (!ignore_frame_change)
    {
      // This doesn't seem right any more. Need to check what the Unity viewer did with this. It may
      // be an artifact of the main thread needing to do so much work in Unity.
      _frame.current = msg.value32;
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
  const std::scoped_lock guard(_data_mutex);

  target_frame = 0;
  if (_frame.pending_target.has_value())
  {
    // We have a new target frame. Migrate to the target frame.
    _frame.target = target_frame = *_frame.pending_target;
    _frame.pending_target.reset();

    // Mustn't call skipToClosestKeyframe() while _data_mutex is locked.
    return TargetFrameState::KeyframeSkip;
  }

  if (!_frame.target.has_value())
  {
    return TargetFrameState::NotSet;
  }

  target_frame = *_frame.target;
  const auto current_frame = _frame.current.load();

  if (target_frame < current_frame)
  {
    return TargetFrameState::Behind;
  }

  if (target_frame > current_frame)
  {
    return TargetFrameState::Ahead;
  }

  _frame.target.reset();
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

  // Keep processing key frames so long as we fail to load one.
  bool keyframe_ok = false;
  bool keyframes_available = true;
  while (keyframes_available && !keyframe_ok)
  {
    keyframes_available = _keyframes.store->lookupNearest(target_frame, keyframe);
    if (!keyframes_available)
    {
      continue;
    }

    if (keyframe.frame_number <= _frame.current && target_frame >= _frame.current)
    {
      // Keyframe is in the past. Ignore it and continue from the current frame.
      return;
    }

    _tes->reset();

    keyframe_ok = loadSnapshot(keyframe.snapshot_path);
    if (!keyframe_ok)
    {
      // Delete the keyframe.
      _keyframes.store->remove(keyframe.frame_number);
      continue;
    }

    // Success. Set steam position.
    log::info("Restore keyframe snapshot for target frame ", target_frame, " to frame ",
              keyframe.frame_number, " at stream pos ", keyframe.position);
    _stream_reader->seek(keyframe.position);

    // Make sure we flag the end of the snapshot frame event.
    _frame.current = keyframe.frame_number;
    _tes->updateToFrame(keyframe.frame_number);
  }

  // Fallback to start of stream.
  if (target_frame < _frame.current.load())
  {
    // No keyframe available or failed to load. Skip to stream start.
    _tes->reset();
    _stream_reader->seek(0);
    _frame.current = 0;
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
