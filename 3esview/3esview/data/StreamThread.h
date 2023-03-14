#ifndef TES_VIEW_STREAM_THREAD_H
#define TES_VIEW_STREAM_THREAD_H

#include <3esview/ViewConfig.h>

#include "DataThread.h"

#include <3esview/FrameStamp.h>
#include <3esview/util/Enum.h>

#include <3escore/Messages.h>

#include <array>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <filesystem>
#include <istream>
#include <memory>
#include <mutex>
#include <optional>
#include <thread>

namespace tes
{
class CollatedPacketDecoder;
class PacketBuffer;
class PacketReader;
class PacketStreamReader;
}  // namespace tes

namespace tes::view
{
class ThirdEyeScene;
}  // namespace tes::view

namespace tes::view::data
{
class KeyframeStore;

/// A @c DataThread implementation which reads and processes packets form a file.
class TES_VIEWER_API StreamThread : public DataThread
{
public:
  using Clock = std::chrono::steady_clock;

  StreamThread(std::shared_ptr<ThirdEyeScene> tes, std::shared_ptr<std::istream> stream);
  ~StreamThread() override;

  /// Reports whether the current stream is a live connection or a replay.
  ///
  /// Live streams do not support playback controls such as pausing and stepping.
  /// @return True if this is a live stream.
  bool isLiveStream() const override;

  /// Set the target frame to update to. This represents a frame jump.
  ///
  /// Threadsafe.
  /// @param frame The frame to jump, skip or step to.
  void setTargetFrame(FrameNumber frame) override;

  /// Get the target frame to jump to. Zero the current frame is up to date; i.e., this is zero once
  /// the current frame reaches the target frame.
  /// @return The target frame to jump to.
  FrameNumber targetFrame() const override;

  /// Get the current frame number.
  FrameNumber currentFrame() const override { return _frame.current; }

  FrameNumber totalFrames() const override { return _frame.total; }

  void setLooping(bool loop) override;
  bool looping() const override;

  void setPlaybackSpeed(float speed) override;
  float playbackSpeed() const override;

  /// Request the thread to quit. The thread may then be joined.
  void stop() override
  {
    _quit_flag = true;
    unpause();
  }

  /// Check if a quit has been requested.
  /// @return True when a quit has been requested.
  bool stopping() const { return _quit_flag; }

  /// Check if playback is paused.
  /// @return True if playback is paused.
  bool paused() const override { return _paused; }
  /// Pause playback.
  void pause() override;
  /// Unpause and resume playback.
  void unpause() override;

  /// Set if new keyframes are allowed to be taken. Does not affect existing keyframes.
  /// @param allowed True if allowed.
  void setAllowKeyframes(bool allowed);

  /// Query if new keyframes are allowed to be taken.
  /// @return True if new keyframes are allowed.
  bool allowKeyframes() const;

  /// Set the keyframe size interval. Keyframes are taken when either interval elapses.
  /// @param interval_mib The number of MiB which must elapsed before a new keyframe is taken.
  void setKeyframeSizeInterval(size_t interval_mib);

  /// Get the keyframe size interval.
  /// @return The size interval between keyframes - MiB
  size_t keyframeSizeIntervalMiB() const;

  /// Set the keyframe interval. Keyframes are taken when either interval elapses.
  /// @param interval The number of frames between keyframes.
  void setKeyframeInterval(FrameNumber interval);

  /// Set the keyframe interval.
  /// @return The number of frames between each keyframe.
  FrameNumber keyframeInterval() const;

  /// Set the minimum keyframe interval. Keyframes are not allowed unless at least this many frames
  /// has elapsed. Then either the @c keyframeInterval() or the @c keyframeSizeIntervaMiB() can
  /// trigger a keyframe..
  /// @param interval The number of frames which must elapsed before a new keyframe is taken.
  void setKeyframeMinimumInterval(FrameNumber interval);

  /// Set the minimum keyframe interval.
  /// @return interval The minimum number of frames between each keyframe.
  FrameNumber keyframeMinimumInterval() const;

  /// Wait for this thread to finish.
  void join() override;

  /// Check for packet version number compability.
  /// @param header The packet header to check - expecting network byte order.
  /// @return True if compatible.
  static bool checkCompatibility(const PacketHeader *header);
  /// Check for packet version number compability.
  /// @param reader Packet to check wrapper in a reader.
  /// @return True if compatible.
  static bool checkCompatibility(const PacketReader &reader);

protected:
  /// Thread entry point.
  void run();

  void skipBack(FrameNumber target_frame);

public:
  /// Option flags for @c processPacket()
  /// @note Should be private, but is not to allow enum flag operators to be defined..
  enum class ProcessPacketFlag : unsigned
  {
    /// No special options.
    None = 0,
    /// Suppress the frame end message handling.
    NoFrameEnd = (1u << 0u)
  };

private:
  /// Status for @c processPacket()
  enum class ProcessPacketStatus
  {
    /// An error has occurred
    Error,
    /// We are at the end of a frame and the end frame packet was naked; not part of a collated
    /// packet.
    EndFrameNaked,
    /// We are at the end of a frame.
    EndFrame,
    /// We are in the middle of processing a frame.
    MidFrame
  };

  /// Return value for @c processPacket() .
  struct ProcessPacketResult
  {
    /// The time interval needed before continuing to the next frame. May be zero when not changing.
    StreamThread::Clock::duration frame_interval;
    /// Processing status.
    ProcessPacketStatus status = ProcessPacketStatus::Error;
    /// True if we must reset any pending time intervals and promptly continue to the next frame.
    bool reset_timeline = false;
  };

  /// Block if paused until unpaused.
  /// @return True if we were paused and had to wait.
  bool blockOnPause();

  /// Process the given packet (header), allowing that it may be a collated packet.
  ///
  /// This takes the header and gives it to the @c packet_decoder. Then while
  /// @c CollatedPacketDecoder::next() is valid, each packet is checked for compability, then
  /// processed.
  ///
  /// The return value identifies any frame time interval indicated by the processed packet(s) and
  /// whether a time line reset is required (no sleep). A general @c ProcessPacketStatus is also
  /// provided.
  ///
  /// @param packet_header The packet header pointer.
  /// @param packet_decoder A collated packet decoder to use in case @p packet_header contains
  /// collated data.
  /// @param flags Processing option flags.
  /// @return A @c ProcessPacketResult which identifies the processing outcome.
  ProcessPacketResult processPacket(const PacketHeader *packet_header,
                                    CollatedPacketDecoder &packet_decoder,
                                    ProcessPacketFlag flags = ProcessPacketFlag::None);

  /// Process a control packet.
  ///
  /// This covers end of frame events, so the return value indicates how long to delay before the
  /// next frame.
  ///
  /// Handles the following messages:
  /// - @c CIdFrame increments @p _currentFrame then calls @c ThirdEyeScene::updateToFrame() .
  /// - @c CIdCoordinateFrame updates @c _server_info then calls @c
  /// ThirdEyeScene::updateServerInfo() .
  /// - @c CIdFrameCount updates @c _frame.total.
  /// - @c CIdForceFrameFlush calls @c ThirdEyeScene::updateToFrame() with the @p _currentFrame.
  /// - @c CIdReset resets the @c _currentFrame and calls @c ThirdEyeScene::reset() .
  /// - @c CIdKeyframe - NYI
  /// - @c CIdEnd - NYI
  ///
  /// @param packet The packet to control. The routing Id is always @c MtControl.
  /// @param ignore_frame_change True to ignore frame change messages. Used when loading snapshots.
  /// @return The time to delay before the next frame, or a zero duration when not a frame message.
  Clock::duration processControlMessage(PacketReader &packet, bool ignore_frame_change);

  /// Return values for @c checkTargetFrameState()
  enum class TargetFrameState
  {
    NotSet,        ///< No target frame set
    KeyframeSkip,  ///< Need to skip to keyframe nearest the  target frame before checking again.
    Behind,  ///< Target frame is set and behind the current frame. Requires keyframe or file reset.
    Ahead,   ///< The target frame is ahead of the current frame.
    Reached  ///< Target frame has just been reached.
  };

  /// Check the conditions around the target frame being set.
  ///
  /// This checks the @c targetFrame() value to see if we need to adjust playback to reach the
  /// target frame. The current state is indicated by the @c TargetFrameState return value.
  ///
  /// - @c TargetFrameState::NotSet : the target frame is not set and we use normal playback rules.
  /// - @c TargetFrameState::Behind : the target frame set behind the current frame. We must reset
  ///   to a keyframe (or the file start) and catch up to the desired frame. After the reset the
  ///   next check will be @c TargetFrameState::Ahead until the frame is
  ///   @c TargetFrameState::Reached
  /// - @c TargetFrameState::Ahead : the target frame is set ahead of the current frame and we need
  ///   to process messages to catch up to the target frame.
  /// - @c TargetFrameState::Reached : the target frame has been reached and we can result normal
  ///   playback. This also clears the @c targetFrame() value so the next call will return
  ///   @c TargetFrameState::NotSet
  ///
  /// @param[out] target_frame Set to the target frame value. Only useful when returning
  /// @c TargetFrameState::Valid
  /// @return Details of the @c targetFrameValue() - see comments.
  TargetFrameState checkTargetFrameState(FrameNumber &target_frame);

  /// Check if we need to take a keyframe snapshot. Only to be called at the end of a frame.
  /// @param frame_number The frame number of the frame just beginning.
  /// @param stream_position The file stream position at which this frame data begins.
  /// @return True if a keyframe is required.
  bool keyframeNeeded(FrameNumber frame_number, std::istream::pos_type stream_position) const;
  /// Make a keyframe snapshot for the ending frame. Only to be called at the end of a frame.
  /// @param frame_number The frame number of the frame just beginning.
  /// @param stream_position The file stream position at which this frame data begins.
  /// @return True if the snapshot is successful.
  bool makeKeyframe(FrameNumber frame_number, std::istream::pos_type stream_position);
  /// Skip the stream to the closest keyframe before @p target_frame . This may mean resetting
  /// to the beginning of the file stream. The stream is unchanged if the current frame equals or
  /// exceeds the last keyframe.
  /// @param target_frame The target frame number.
  void skipToClosestKeyframe(FrameNumber target_frame);
  /// Load a snapshot from the given path and process it's packets.
  /// @param snapshot_path The snapshot path.
  /// @return True on success.
  bool loadSnapshot(const std::filesystem::path &snapshot_path);

  /// Keyframe data. Keyframes are to be made whenever the specified number of frames elapses and
  /// the specified number of MiB are processed from the data stream.
  struct Keyframes
  {
    /// Keyframe storage
    std::unique_ptr<KeyframeStore> store;
    /// Data at which to make a new keyframe (MiB).
    size_t size_interval_mib = 20;  // NOLINT(readability-magic-numbers)
    /// Frame number interval at which to make keyframes.
    FrameNumber frame_interval = 100;  // NOLINT(readability-magic-numbers)
    /// Minimum number of frames between key frames..
    FrameNumber frame_minimum_interval = 5;  // NOLINT(readability-magic-numbers)
    /// True if new keyfames are allowed.
    bool enabled = true;
  };

  /// Tracks details about frame counts and targets.
  struct FrameState
  {
    /// The current frame.
    FrameNumberAtomic current = 0;
    /// The total number of frames in the stream, if know. Zero when unknown.
    FrameNumberAtomic total = 0;
    /// The target frame we are trying to get to. Commited from @c _pending_target once that value
    /// has been accepted.
    std::optional<FrameNumber> target;
    /// Pending value for @c target. We use a pending value as a kind of hysteresis against spamming
    /// @c setTargetFrame() .
    std::optional<FrameNumber> pending_target;
    /// True whenever the @c current frame is catching up to the @c target frame.
    bool catching_up = false;
  };

  mutable std::mutex _data_mutex;
  std::condition_variable _notify;
  FrameState _frame;
  std::atomic_bool _quit_flag;
  std::atomic_bool _paused;
  bool _looping = false;
  float _playback_speed = 1.0f;
  std::unique_ptr<PacketStreamReader> _stream_reader;
  /// The scene manager.
  std::shared_ptr<ThirdEyeScene> _tes;
  std::thread _thread;
  ServerInfoMessage _server_info;
  bool _have_server_info = false;
  Keyframes _keyframes;
};
}  // namespace tes::view::data

TES_ENUM_FLAGS(tes::view::data::StreamThread::ProcessPacketFlag, unsigned);

#endif  // TES_VIEW_STREAM_THREAD_H
