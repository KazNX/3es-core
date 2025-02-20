#pragma once

#include <3esview/ViewConfig.h>

#include <3escore/CoordinateFrame.h>

#include <Magnum/Magnum.h>
#include <Magnum/Math/Matrix4.h>
#include <Magnum/Math/Vector3.h>

namespace tes::view::camera
{
struct TES_VIEWER_API Camera
{
  static constexpr float kDefaultFov = 70.0f;
  static constexpr float kDefaultClipNear = 0.1f;
  static constexpr float kDefaultClipFar = 1000.0f;

  Magnum::Vector3 position = {};
  float pitch = 0;
  float yaw = 0;
  float fov_horizontal_deg = kDefaultFov;
  float clip_near = kDefaultClipNear;
  float clip_far = kDefaultClipFar;
  // TODO: apply this frame. For now just use XYZ.
  tes::CoordinateFrame frame = tes::CoordinateFrame::XYZ;
};

/// Calculate the camera world transform. This is in X right, Y forward, Z up.
inline Magnum::Matrix4 matrix(const Camera &camera)
{
  Magnum::Matrix4 transform = Magnum::Matrix4::translation(camera.position) *        //
                              Magnum::Matrix4::rotationZ(Magnum::Rad(camera.yaw)) *  //
                              Magnum::Matrix4::rotationX(Magnum::Rad(camera.pitch));
  return transform;
}

/// Calculate the camera view matrix.
inline Magnum::Matrix4 view(const Camera &camera)
{
  Magnum::Matrix4 frame_transform;
  // Build the transform to map into (OpenGL/Vulkan) view space: X right, -Z forward, Y up.
  switch (camera.frame)
  {
  // TODO(KS): fix the other coordinate frames.
  default:
  case tes::CoordinateFrame::XYZ:
    frame_transform = { { 1, 0, 0, 0 },   //
                        { 0, 0, -1, 0 },  //
                        { 0, 1, 0, 0 },   //
                        { 0, 0, 0, 1 } };
    break;
  }
  return frame_transform * matrix(camera).inverted();
}


/// Generate the projection matrix.
inline Magnum::Matrix4 projection(const Camera &camera, const Magnum::Vector2i &view_size)
{
  return Magnum::Matrix4::perspectiveProjection(Magnum::Math::Deg(camera.fov_horizontal_deg),
                                                Magnum::Vector2(view_size).aspectRatio(),
                                                camera.clip_near, camera.clip_far);
}


/// Generate the camera projection * view matrix.
inline Magnum::Matrix4 viewProjection(const Camera &camera, const Magnum::Vector2i &view_size)
{
  const Magnum::Matrix4 projection = Magnum::Matrix4::perspectiveProjection(
    Magnum::Math::Deg(camera.fov_horizontal_deg), Magnum::Vector2(view_size).aspectRatio(),
    camera.clip_near, camera.clip_far);
  const Magnum::Matrix4 camera_transform = view(camera);
  return projection * camera_transform;
}


}  // namespace tes::view::camera
