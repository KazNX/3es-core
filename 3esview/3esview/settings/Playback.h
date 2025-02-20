//
// Author: Kazys Stepanas
//
#pragma once

#include <3esview/ViewConfig.h>

#include "Values.h"

namespace tes::view::settings
{
// NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers)
struct Playback
{
  Boolean allow_key_frames = {
    "Allow key frames", false,
    "Enable scene keyframes to cache frames during playback and stepping?"
  };
  UInt keyframe_every_mib = {
    "Keyframe every MiB", 20, 1, 1024 * 1024,
    "Create a keyframe after reading this many mibibytes from the playback stream."
  };
  UInt keyframe_every_frames = { "Keyframe every X frames", 100, 1, 100000,
                                 "Create a keyframe after this number of frames elapses." };
  UInt keyframe_min_separation = {
    "Key frame minimum separation", 5, 1, 1000,
    "Do not allow keyframes unless this number of frames has elapsed."
  };
  Boolean keyframe_compression = { "Keyframe compression", true, "Compress key frames?" };
  Boolean looping = { "Looping", false,
                      "Automatically restart playback at the end of a file stream?" };
  Boolean pause_on_error = {
    "Pause on error", true, "Pause if an error occurs during playback? Only affects file playback."
  };

  [[nodiscard]] inline bool operator==(const Playback &other) const
  {
    return allow_key_frames == other.allow_key_frames &&
           keyframe_every_mib == other.keyframe_every_mib &&
           keyframe_every_frames == other.keyframe_every_frames &&
           keyframe_min_separation == other.keyframe_min_separation &&
           keyframe_compression == other.keyframe_compression && looping == other.looping &&
           pause_on_error == other.pause_on_error;
  }

  [[nodiscard]] inline bool operator!=(const Playback &other) const { return !operator==(other); }
};
// NOLINTEND(cppcoreguidelines-avoid-magic-numbers)
}  // namespace tes::view::settings
