//
// Author: Kazys Stepanas
//
#pragma once

#include <3esview/ViewConfig.h>

#include "Values.h"

namespace tes::view::settings
{
// NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers)
struct Camera
{
  Boolean invert_y = { "Invert Y", false, "Invert mouse Y axis?" };
  Boolean allow_remote_settings = { "Allow remote", true,
                                    "Use remote clip plane and field of view settings?" };
  Float near_clip = { "Near clip", 0.1f, 0.0f, 100.0f,
                      "The default near clip plane when not using remote settings." };
  Float far_clip = { "Far clip", 2000.0f, 0.0f, 3000.0f,
                     "The default far clip plane when not using remote settings." };
  Float fov = { "Field of view", 60.0f, 0.0f, 180.0f,
                "The default horizontal field of view (degrees)." };

  [[nodiscard]] inline bool operator==(const Camera &other) const
  {
    return invert_y == other.invert_y && allow_remote_settings == other.allow_remote_settings &&
           near_clip == other.near_clip && far_clip == other.far_clip && fov == other.fov;
  }

  [[nodiscard]] inline bool operator!=(const Camera &other) const { return !operator==(other); }
};
// NOLINTEND(cppcoreguidelines-avoid-magic-numbers)
}  // namespace tes::view::settings
