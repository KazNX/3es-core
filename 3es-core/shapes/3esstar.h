//
// author: Kazys Stepanas
//
#ifndef _3ESSTAR_H_
#define _3ESSTAR_H_

#include "3es-core.h"
#include "3esshape.h"

namespace tes
{
/// Defines a star to display. A star is a shape with extrusions in both directions along each axis with spherical
/// extents.
///
/// A star is defined by:
/// Component      | Description
/// -------------- | -----------------------------------------------------------------------------------------------
/// @c centre()    | The star centre. An alias for @p position().
/// @c radius()    | The star radius.
class _3es_coreAPI Star : public Shape
{
public:
  /// Create a star.
  /// @param id The shape id and category, with unique id among @c Star objects, or zero for a transient shape.
  /// @param transform The spherical transform for the star.
  Star(const ShapeId &id = ShapeId(), const Spherical &transform = Spherical());

  /// Create a fully scale star. This constructor allows for scaling and rotating the star.
  /// @param id The shape id and category, with unique id among @c Star objects, or zero for a transient shape.
  /// @param transform An arbitrary transform for the shape, supporting non-uniform scaling.
  Star(const ShapeId &id, const Transform &transform);

  inline const char *type() const override { return "star"; }

  /// Set the star radial extents.
  /// @param radius The star radius.
  /// @return @c *this
  Star &setRadius(double radius);
  /// Get the star radial extents.
  /// @return The star radius.
  double radius() const;

  /// Set the star centre coordinate.
  /// @param centre The new star centre.
  /// @return @c *this
  Star &setCentre(const Vector3d &centre);
  /// Get the star centre coordinate.
  /// @return The star centre.
  Vector3d centre() const;
};


inline Star::Star(const ShapeId &id, const Spherical &transform)
  : Shape(SIdStar, id, transform)
{
}


inline Star::Star(const ShapeId &id, const Transform &transform)
  : Shape(SIdStar, id, transform)
{
}


inline Star &Star::setRadius(double radius)
{
  setScale(Vector3d(radius));
  return *this;
}


inline double Star::radius() const
{
  return scale().x;
}


inline Star &Star::setCentre(const Vector3d &centre)
{
  setPosition(centre);
  return *this;
}


inline Vector3d Star::centre() const
{
  return position();
}
}  // namespace tes

#endif  // _3ESSTAR_H_
