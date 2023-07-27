// Author: Kazys Stepanas
#ifndef TES_VIEW_CONSTANTS_H
#define TES_VIEW_CONSTANTS_H

#include "ViewConfig.h"

#include <3escore/CoordinateFrame.h>
#include <3escore/Vector3.h>

namespace tes::view
{
/// Specifies the coordinate frame of the client viewer.
constexpr CoordinateFrame kViewerCoordinateFrame = tes::CoordinateFrame::XYZ;

/// Get the world right vector for the client viewer.
/// @tparam Real @c float or @c double
/// @return The world right vector.
template <typename Real>
constexpr Vector3<Real> worldRight()
{
  return Vector3<Real>(static_cast<Real>(1), 0, 0);
}

/// Get the world forward vector for the client viewer.
/// @tparam Real @c float or @c double
/// @return The world forward vector.
template <typename Real>
constexpr Vector3<Real> worldForward()
{
  return Vector3<Real>(0, static_cast<Real>(1), 0);
}

/// Get the world up vector for the client viewer.
/// @tparam Real @c float or @c double
/// @return The world up vector.
template <typename Real>
constexpr Vector3<Real> worldUp()
{
  return Vector3<Real>(0, 0, static_cast<Real>(1));
}
}  // namespace tes::view

#endif  // TES_VIEW_CONSTANTS_H
