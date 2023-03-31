#include "SetCamera.h"

#include <3esview/Viewer.h>

#include <3esview/data/DataThread.h>
#include <3esview/handler/Camera.h>

namespace tes::view::command::camera
{
SetCamera::SetCamera()
  : Command("setCamera", Args(kFreeCameraId))
{}


bool SetCamera::checkAdmissible(Viewer &viewer) const
{
  const auto data_thread = viewer.dataThread();
  return data_thread != nullptr && !data_thread->isLiveStream();
}


CommandResult SetCamera::invoke(Viewer &viewer, const ExecInfo &info, const Args &args)
{
  (void)info;
  auto camera_id_int = arg<int>(0, args);
  // Limit to the expected range.
  using CameraId = handler::Camera::CameraId;
  if (camera_id_int != kFreeCameraId)
  {
    // Clamp to prevent exceptions.
    camera_id_int =
      std::max<int>(std::numeric_limits<CameraId>::min(),
                    std::min<int>(camera_id_int, std::numeric_limits<CameraId>::max()));
    viewer.setActiveCamera(static_cast<CameraId>(camera_id_int));
  }
  else
  {
    viewer.clearActiveCamera();
  }
  return { CommandResult::Code::Ok };
}
}  // namespace tes::view::command::camera
