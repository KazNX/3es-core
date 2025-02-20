#pragma once

#include "3esview/ViewConfig.h"

#include <3escore/Colour.h>

#include <Magnum/Magnum.h>
#include <Magnum/Math/Color.h>

namespace tes::view
{
inline tes::Colour convert(const Magnum::Color3 &c)
{
  return { c.x(), c.y(), c.z() };
}

inline tes::Colour convert(const Magnum::Color4 &c)
{
  return { c.x(), c.y(), c.z(), c.w() };
}

inline Magnum::Color4 convert(const tes::Colour &c)
{
  return { Magnum::Float(c.rf()), Magnum::Float(c.gf()), Magnum::Float(c.bf()),
           Magnum::Float(c.af()) };
}
};  // namespace tes::view
