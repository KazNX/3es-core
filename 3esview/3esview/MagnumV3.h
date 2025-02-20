#pragma once

#include "3esview/ViewConfig.h"

#include <3escore/Vector3.h>

#include <Magnum/Magnum.h>
#include <Magnum/Math/Vector3.h>

namespace tes::view
{
inline tes::Vector3<Magnum::Float> convert(const Magnum::Vector3 &v)
{
  return { v.x(), v.y(), v.z() };
}

template <typename T>
inline Magnum::Vector3 convert(const tes::Vector3<T> &v)
{
  return { Magnum::Float(v.x()), Magnum::Float(v.y()), Magnum::Float(v.z()) };
}
};  // namespace tes::view
