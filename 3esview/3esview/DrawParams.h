//
// Author: Kazys Stepanas
//
#ifndef TES_VIEW_DRAW_PARAMS_H
#define TES_VIEW_DRAW_PARAMS_H

#include "3esview/ViewConfig.h"

#include "camera/Camera.h"

#include <Magnum/Math/Matrix3.h>
#include <Magnum/Math/Matrix4.h>
#include <Magnum/Math/Vector2.h>

namespace tes::view
{
/// Render related parameters passed to the @c Message::draw() function.
struct TES_VIEWER_API DrawParams
{
  /// Current view camera.
  camera::Camera camera = {};
  /// The current projection matrix. This does not include the view matrix.
  Magnum::Matrix4 projection_matrix = {};
  /// The inverse of @p camera_matrix.
  Magnum::Matrix4 view_matrix = {};
  /// Represents the @c camera transform in the world.
  Magnum::Matrix4 camera_matrix = {};
  /// Transformation from world space to the projection: `projection_matrix * view_matrix`.
  Magnum::Matrix4 pv_transform = {};
  /// Size of the viewport being drawn to (pixels).
  Magnum::Vector2i view_size = {};

  DrawParams() = default;
  DrawParams(const camera::Camera &camera, const Magnum::Vector2i &view_size)
    : camera(camera)
    , projection_matrix(camera::projection(camera, view_size))
    , view_matrix(camera::view(camera))
    , camera_matrix(camera::matrix(camera))
    , pv_transform(projection_matrix * view_matrix)
    , view_size(view_size)
  {}
};
}  // namespace tes::view

#endif  // TES_VIEW_DRAW_PARAMS_H
