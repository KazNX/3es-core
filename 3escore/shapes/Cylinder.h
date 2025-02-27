//
// author: Kazys Stepanas
//
#pragma once

#include <3escore/CoreConfig.h>

#include "Shape.h"

namespace tes
{
/// Defines a cylinder shape to display.
///
/// An arrow is defined by:
/// Component      | Description
/// -------------- |
/// -----------------------------------------------------------------------------------------------
/// @c centre()    | The centre of the cylinder. Alias for @p position().
/// @c axis()      | Defines the cylinder primary axis. Affects @c rotation().
/// @c length()    | The length of the cylinder body.
/// @c radius()    | Radius of the cylinder walls.
class TES_CORE_API Cylinder : public Shape
{
public:
  /// Construct a cylinder object.
  /// @param id The shape id and category, unique among @c Capsule objects, or zero for a transient
  /// shape.
  /// @param transform The directional transformation for the capsule.
  Cylinder(const Id &id = Id(), const Directional &transform = Directional());

  /// Construct a cylinder object.
  /// @param id The shape id and category, unique among @c Capsule objects, or zero for a transient
  /// shape.
  /// @param transform An arbitrary transform for the shape, supporting non-uniform scaling.
  Cylinder(const Id &id, const Transform &transform);

  /// Copy constructor
  /// @param other Object to copy.
  Cylinder(const Cylinder &other) = default;

  /// Move constructor.
  /// @param other Object to move.
  Cylinder(Cylinder &&other) noexcept = default;

  ~Cylinder() override = default;

  /// Copy assignment.
  /// @param other The object to copy.
  /// @return @c *this
  Cylinder &operator=(const Cylinder &other) noexcept = default;

  /// Move assignment.
  /// @param other The object to move.
  /// @return @c *this
  Cylinder &operator=(Cylinder &&other) noexcept = default;

  [[nodiscard]] const char *type() const override { return "cylinder"; }

  /// Set the cylinder body radius.
  /// @param radius The radius to set.
  /// @return @c *this
  Cylinder &setRadius(double radius);
  /// Get the cylinder radius.
  /// @return The cylinder radius.
  [[nodiscard]] double radius() const;

  /// Set the cylinder body length.
  /// @param length The body length to set.
  /// @return @c *this
  Cylinder &setLength(double length);
  /// Get the cylinder body length.
  /// @param The body length.
  [[nodiscard]] double length() const;

  /// Set the position fo the cylinder centre.
  /// @param centre The centre coordinate.
  /// @return @c *this
  Cylinder &setCentre(const Vector3d &centre);
  /// Get the cylinder centre position.
  /// @return The centre coordinate.
  [[nodiscard]] Vector3d centre() const;

  /// Set the cylinder primary axis. Affects @p rotation().
  /// @param axis The new axis to set.
  /// @return @c *this
  Cylinder &setAxis(const Vector3d &axis);
  /// Get the cylinder primary axis.
  ///
  /// May not exactly match the axis given via @p setAxis() as the axis is defined by the quaternion
  /// @c rotation().
  /// @return The primary axis.
  [[nodiscard]] Vector3d axis() const;
};


inline Cylinder::Cylinder(const Id &id, const Directional &transform)
  : Shape(SIdCylinder, id, transform)
{}


inline Cylinder::Cylinder(const Id &id, const Transform &transform)
  : Shape(SIdCylinder, id, transform)
{}


inline Cylinder &Cylinder::setRadius(double radius)
{
  Vector3d s = Shape::scale();
  s.x() = s.y() = radius;
  setScale(s);
  return *this;
}


inline double Cylinder::radius() const
{
  return scale().x();
}


inline Cylinder &Cylinder::setLength(double length)
{
  Vector3d s = Shape::scale();
  s.z() = length;
  setScale(s);
  return *this;
}


inline double Cylinder::length() const
{
  return scale().z();
}


inline Cylinder &Cylinder::setCentre(const Vector3d &centre)
{
  setPosition(centre);
  return *this;
}


inline Vector3d Cylinder::centre() const
{
  return position();
}


inline Cylinder &Cylinder::setAxis(const Vector3d &axis)
{
  Quaterniond rot;
  const double dir_deviation = 0.9998;
  if (axis.dot(Directional::DefaultDirection) > -dir_deviation)
  {
    rot = Quaterniond(Directional::DefaultDirection, axis);
  }
  else
  {
    rot.setAxisAngle(Vector3d::AxisX, M_PI);
  }
  setRotation(rot);
  return *this;
}


inline Vector3d Cylinder::axis() const
{
  const Quaterniond rot = rotation();
  return rot * Directional::DefaultDirection;
}
}  // namespace tes
