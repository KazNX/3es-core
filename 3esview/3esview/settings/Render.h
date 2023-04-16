//
// Author: Kazys Stepanas
//
#ifndef TES_VIEW_SETTINGS_RENDER_H
#define TES_VIEW_SETTINGS_RENDER_H

#include <3esview/ViewConfig.h>

#include "Values.h"

namespace tes::view::settings
{
struct Render
{
  Bool use_edl_shader = { "EDL shader", true, "Enable Eye-Dome-Lighting shader?" };
  UInt edl_radius = { "EDL radius", 1, 1, 10, "The pixel search radius used in EDL calculations." };
  Float edl_exponential_scale = { "EDL exponential scale", 3.0f, 0.1f, 30.0f,
                                  "Exponential scaling for EDL shader." };
  Float edl_linear_scale = { "EDL linear scale", 1.0f, 1.0f, 10.0f,
                             "Linear scaling for EDL shader." };
  Float point_size = { "Point size", 4.0f, 1.0f, 64.0f,
                       "Point render size for point clouds (pixels)." };
  Colour background_colour = { "Background colour", tes::Colour::Grey, "Background colour." };

  [[nodiscard]] inline bool operator==(const Render &other) const
  {
    return use_edl_shader == other.use_edl_shader && edl_radius == other.edl_radius &&
           edl_exponential_scale == other.edl_exponential_scale &&
           edl_linear_scale == other.edl_linear_scale && point_size == other.point_size &&
           background_colour == other.background_colour;
  }

  [[nodiscard]] inline bool operator!=(const Render &other) const { return !operator==(other); }
};
}  // namespace tes::view::settings

#endif  // TES_VIEW_SETTINGS_RENDER_H
