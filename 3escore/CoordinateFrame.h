//
// author: Kazys Stepanas
//
#pragma once

#include "CoreConfig.h"

namespace tes
{
/// Enumerates various coordinate frames. Each frame specifies the global axes in
/// the order right, forward, up. The up axis may be negated, that is a positive
/// value is down, in which case the 'Neg' suffix is added.
///
/// Right handed coordinate frames come first with left handed frames being those
/// greater than or equal to @c Left.
///
/// Examples:
/// Label | Right | Forward | Up  | Notes
/// ----- | ----- | ------- | --- | ----------------------------------------------
/// XYZ   | X     | Y       | Z   | A common extension of 2D Catesian coordinates.
/// XZY   | X     | Z       | Y   | The default in Unity 3D.
/// XZYNeg| X     | Z       | -Y  | A common camera space system.
enum class CoordinateFrame : uint8_t
{
  // Right handled frames.
  XYZ,
  XZYNeg,
  YXZNeg,
  YZX,
  ZXY,
  ZYXNeg,
  // Left handed frames
  XYZNeg,
  XZY,
  YXZ,
  YZXNeg,
  ZXYNeg,
  ZYX,

  Count,
  Left = XYZNeg
};
}  // namespace tes
