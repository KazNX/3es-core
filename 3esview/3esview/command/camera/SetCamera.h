#ifndef TES_VIEW_COMMAND_CAMERA_SET_CAMERA_H
#define TES_VIEW_COMMAND_CAMERA_SET_CAMERA_H

#include <3esview/ViewConfig.h>

#include <3esview/command/Command.h>

namespace tes::view::command::camera
{
class TES_VIEWER_API SetCamera : public Command
{
public:
  SetCamera();

protected:
  bool checkAdmissible(Viewer &viewer) const override;
  CommandResult invoke(Viewer &viewer, const ExecInfo &info, const Args &args) override;
};
}  // namespace tes::view::command::camera

#endif  // TES_VIEW_COMMAND_CAMERA_SET_CAMERA_H
