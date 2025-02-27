//
// author: Kazys Stepanas
//
#pragma once

#include <3escore/CoreConfig.h>

#include "Shape.h"

namespace tes
{
/// Defines an arrow shape to display.
///
/// An arrow is defined by:
/// Component      | Description
/// -------------- |
/// -----------------------------------------------------------------------------------------------
/// @c origin()    | The arrow base position. Alias for @p position().
/// @c direction() | The arrow direction vector. Must be unit length.
/// @c length()    | Length of the arrow from base to tip.
/// @c radius()    | Radius of the arrow body. The arrow head will be slightly larger.
class TES_CORE_API Arrow : public Shape
{
public:
  /// Construct an arrow object.
  /// @param id The shape id and category, unique among @c Arrow objects, or zero for a transient
  /// shape.
  /// @param transform The directional transformation for the shape.
  Arrow(const Id &id = Id(), const Directional &transform = Directional());

  /// Construct an arrow object.
  /// @param id The shape id and category, unique among @c Arrow objects, or zero for a transient
  /// shape.
  /// @param transform An arbitrary transform for the shape, supporting non-uniform scaling.
  Arrow(const Id &id, const Transform &transform);

  /// Copy constructor
  /// @param other Object to copy.
  Arrow(const Arrow &other) = default;

  /// Move constructor.
  /// @param other Object to move.
  Arrow(Arrow &&other) noexcept = default;

  ~Arrow() override = default;

  /// Copy assignment.
  /// @param other The object to copy.
  /// @return @c *this
  Arrow &operator=(const Arrow &other) noexcept = default;

  /// Move assignment.
  /// @param other The object to move.
  /// @return @c *this
  Arrow &operator=(Arrow &&other) noexcept = default;

  [[nodiscard]] const char *type() const override { return "arrow"; }

  /// Set the arrow radius.
  /// @param radius The new arrow radius.
  /// @return @c *this
  Arrow &setRadius(double radius);
  /// Get the arrow radius. Defines the shaft radius, while the head flanges to a sightly larger
  /// radius.
  /// @return The arrow body radius.
  [[nodiscard]] double radius() const;

  /// Set the arrow length from base to tip.
  /// @param length Set the length to set.
  /// @return @c *this
  Arrow &setLength(double length);
  /// Get the arrow length from base to tip.
  /// @return The arrow length.
  [[nodiscard]] double length() const;

  /// Set the arrow origin. This is the arrow base position.
  ///
  /// Note: this aliases @p setPosition().
  ///
  /// @param origin The arrow base position.
  /// @return @c *this
  Arrow &setOrigin(const Vector3d &origin);

  /// Get the arrow base position.
  ///
  /// Note: this aliases @c position().
  /// @return The arrow base position.
  [[nodiscard]] Vector3d origin() const;

  /// Set the arrow direction vector.
  /// @param direction The direction vector to set. Must be unit length.
  /// @return @c *this
  Arrow &setDirection(const Vector3d &direction);
  /// Get the arrow direction vector.
  ///
  /// May not exactly match the axis given via @p setDirection() as the direction is defined by the
  /// quaternion
  /// @c rotation().
  /// @return The arrow direction vector.
  [[nodiscard]] Vector3d direction() const;
};


inline Arrow::Arrow(const Id &id, const Directional &transform)
  : Shape(SIdArrow, id, transform)
{}


inline Arrow::Arrow(const Id &id, const Transform &transform)
  : Shape(SIdArrow, id, transform)
{}


inline Arrow &Arrow::setRadius(double radius)
{
  Vector3d s = Shape::scale();
  s.x() = s.y() = radius;
  setScale(s);
  return *this;
}


inline double Arrow::radius() const
{
  return scale().x();
}


inline Arrow &Arrow::setLength(double length)
{
  Vector3d s = Shape::scale();
  s.z() = length;
  setScale(s);
  return *this;
}


inline double Arrow::length() const
{
  return scale().z();
}


inline Arrow &Arrow::setOrigin(const Vector3d &origin)
{
  setPosition(origin);
  return *this;
}


inline Vector3d Arrow::origin() const
{
  return position();
}


inline Arrow &Arrow::setDirection(const Vector3d &direction)
{
  Quaterniond rot;
  const double dir_deviation = 0.9998;
  if (direction.dot(Directional::DefaultDirection) > -dir_deviation)
  {
    rot = Quaterniond(Directional::DefaultDirection, direction);
  }
  else
  {
    rot.setAxisAngle(Vector3d::AxisX, M_PI);
  }
  setRotation(rot);
  return *this;
}


inline Vector3d Arrow::direction() const
{
  const Quaterniond rot = rotation();
  return rot * Directional::DefaultDirection;
}
}  // namespace tes
