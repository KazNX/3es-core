#pragma once

#include <3esview/ViewConfig.h>

#include <3esview/command/Command.h>

namespace tes::view::command::camera
{
/// Command to change the active camera.
///
/// This accepts an int arg. If this arg matches a valid camera ID [0, 255], then that camera is
/// used instead of the free UI camera. If arg matches @c kClearCameraArg, then the free camera is
/// restored. Other out of range values yield undefined behaviour.
///
/// Setting an unknown camera ID is also undefined, but will result in a camera which cannot be user
/// controlled.
class TES_VIEWER_API SetCamera : public Command
{
public:
  /// Argument value to clear the camera, setting a free UI camera.
  static constexpr int kFreeCameraId = -1;

  SetCamera();

protected:
  bool checkAdmissible(Viewer &viewer) const override;
  CommandResult invoke(Viewer &viewer, const ExecInfo &info, const Args &args) override;
};
}  // namespace tes::view::command::camera
