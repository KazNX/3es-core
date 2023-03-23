#include "SetCamera.h"

#include <3esview/Viewer.h>

#include <3esview/data/DataThread.h>
#include <3esview/handler/Camera.h>

namespace tes::view::command::camera
{
SetCamera::SetCamera()
  : Command("setCamera", Args(static_cast<int>(handler::Camera::kInvalidCameraId)))
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
  using CameraId = decltype(handler::Camera::kInvalidCameraId);
  camera_id_int = std::min<int>(std::numeric_limits<CameraId>::min(),
                                std::max<int>(camera_id_int, std::numeric_limits<CameraId>::max()));
  viewer.setActiveCamera(static_cast<CameraId>(camera_id_int));
  return { CommandResult::Code::Ok };
}
}  // namespace tes::view::command::camera
