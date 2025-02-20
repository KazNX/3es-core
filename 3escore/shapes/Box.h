//
// author: Kazys Stepanas
//
#pragma once

#include <3escore/CoreConfig.h>

#include "Shape.h"

namespace tes
{
/// Defines a rectangular prism shape.
///
/// The box is defined by its centre, scale and orientation. The scale defines the full extents from
/// one corner to another.
///
/// A box is defined by:
/// Component      | Description
/// -------------- |
/// -----------------------------------------------------------------------------------------------
/// @c position()  | The box base position.
/// @c scale()     | The box size/scale, where (1, 1, 1) defines a unit box.
/// @c rotation()  | Quaternion rotation to apply to the box.
class TES_CORE_API Box : public Shape
{
public:
  /// Construct a box object.
  /// @param id The shape id and category, with unique id among @c Box objects, or zero for a
  /// transient shape.
  /// @param transform The box transformation matrix. The position is the box centre, while a unit
  /// scale denotes a unit box.
  Box(const Id &id = Id(), const Transform &transform = Transform());

  /// Copy constructor
  /// @param other Object to copy.
  Box(const Box &other) = default;

  /// Move constructor.
  /// @param other Object to move.
  Box(Box &&other) noexcept = default;

  ~Box() override = default;

  /// Copy assignment.
  /// @param other The object to copy.
  /// @return @c *this
  Box &operator=(const Box &other) noexcept = default;

  /// Move assignment.
  /// @param other The object to move.
  /// @return @c *this
  Box &operator=(Box &&other) noexcept = default;

  [[nodiscard]] const char *type() const override { return "box"; }
};


inline Box::Box(const Id &id, const Transform &transform)
  : Shape(SIdBox, id, transform)
{}
}  // namespace tes
