//
// Author: Kazys Stepanas
//

#ifndef TES_VIEW_DATA_KEYFRAME_STORE_H
#define TES_VIEW_DATA_KEYFRAME_STORE_H

#include <3esview/ViewConfig.h>

#include <3esview/FrameStamp.h>

#include <filesystem>
#include <istream>

namespace tes::view::data
{
/// Manages tracking of the active keyframe temporary files and their associated frame numbers.
///
/// During @c StreamThread playback, we will periodically request a keyframe to represent a snapshot
/// a particular frame number. Keyframe data is saved to a temporary file. Later, when stepping back
/// we restore the closest keyframe before the target frame (inclusive) and begin stream replay from
/// that frame.
///
/// A keyframe has three parts:
///
/// - A frame number
/// - A @c StreamThread file position
/// - A (temporary) keyframe snapshot file.
class TES_VIEWER_API KeyframeStore
{
public:
  struct Keyframe
  {
    FrameNumber frame_number;
    std::istream::pos_type position;
    std::filesystem::path snapshot_path;
  };

  KeyframeStore() = default;
  KeyframeStore(const KeyframeStore &) = delete;
  KeyframeStore(KeyframeStore &&) = default;

  ~KeyframeStore() noexcept;

  KeyframeStore &operator=(const KeyframeStore &) = delete;
  KeyframeStore &operator=(KeyframeStore &&) = default;

  /// Add a keyframe to the store.
  /// @param target_frame The frame number associated with the snapshot and file position.
  /// @param keyframe The keyframe details.
  void add(Keyframe keyframe);

  /// Remove a keyframe. For use when a keyframe fails to restore.
  /// @return True if the keyframe was found and remove.
  bool remove(FrameNumber keyframe_number);

  /// Lookup the keyframe closest to the given @p target_frame .
  ///
  /// This populates @p keyframe with details of the keyframe closest to and preceding
  /// @p target_frame if available. The @p keyframe may be at @p target_frame .
  ///
  /// @param target_frame The frame of interest.
  /// @param[out] keyframe Populated with details about the keyframe.
  /// @return True if @p keyframe is valid after the call.
  bool lookupNearest(FrameNumber target_frame, Keyframe &keyframe) const;

  /// Return the last keyframe details. Zeros if there are no keyframes.
  Keyframe last() const;

  /// Release all the current keyframes.
  void clear();

private:
  using Keyframes = std::vector<Keyframe>;

  /// Search for a keyframe index preceding @p target_number , where the next frame is at or after
  /// @p target_number .
  /// @param target_frame The desired frame number.
  /// @return A pair containing the index and true on success.
  std::pair<size_t, bool> precedingKeyframeIndex(FrameNumber target_frame) const;
  /// Search for a keyframe index exactly matching @p target_number .
  /// @param target_frame The desired frame number.
  /// @return A pair containing the index and true on success.
  std::pair<size_t, bool> exactKeyframeIndex(FrameNumber target_frame) const;

  /// Keyframe set. Note we assume that keyframes are added sequentially and a new keyframe is
  /// always after the previous one in the timeline.
  Keyframes _keyframes;
};
}  // namespace tes::view::data

#endif  // TES_VIEW_DATA_KEYFRAME_STORE_H
