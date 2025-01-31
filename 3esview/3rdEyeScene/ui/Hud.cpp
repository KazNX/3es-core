//
// Author: Kazys Stepanas
//
#include "Hud.h"

#include <3escore/CoreUtil.h>
#include <3escore/Log.h>

#include <3esview/command/camera/SetCamera.h>
#include <3esview/command/Set.h>
#include <3esview/handler/Camera.h>
#include <3esview/data/DataThread.h>
#include <3esview/Viewer.h>

#include <Magnum/GL/TextureFormat.h>
#include <Magnum/ImageView.h>
#include <Magnum/ImGuiIntegration/Context.hpp>
#include <Magnum/Math/Vector2.h>
#include <Magnum/Trade/AbstractImporter.h>
#include <Magnum/Trade/ImageData.h>

#include <Corrade/Containers/Optional.h>
#include <Corrade/PluginManager/PluginManager.h>
#include <Corrade/Utility/Resource.h>

#include <iomanip>
#include <sstream>

namespace tes::view::ui
{

Hud::Hud(Viewer &viewer)
  : Hud(viewer, PreferredCoordinates{ { { 0, 0 }, Anchor::TopLeft, true },
                                      { { 0, 0 }, Stretch::Horizontal | Stretch::Vertical, true } })
{}


Hud::Hud(Viewer &viewer, const PreferredCoordinates &coords)
  : Panel("Hud", viewer)
{
  _preferred_coordinates = coords;
  _window_flags |= ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoMouseInputs;
  _set_camera_command = viewer.commands()->lookupName("setCamera").command;
}


void Hud::drawContent(Magnum::ImGuiIntegration::Context &ui, Window &window)
{
  TES_UNUSED(ui);
  TES_UNUSED(window);

  auto data_thread = _viewer.dataThread();
  if (data_thread)
  {
    drawCameraCombo();
  }

  drawFps();
}


void Hud::drawCameraCombo()
{
  const auto &camera_handler = viewer().tes()->cameraHandler();
  using CameraId = handler::Camera::CameraId;
  std::vector<CameraId> cameras;
  const auto active_camera_id = viewer().activeCamera();
  camera_handler.enumerate(cameras);
  constexpr auto kFreeCameraId = command::camera::SetCamera::kFreeCameraId;

  // Add an entry for the free/fly camera to the data arrays.
  std::vector<std::string> labels(cameras.size() + 1);
  std::vector<int> ids(labels.size());
  std::vector<const char *> labels_cstr(labels.size());

  // Start by adding the free camera.
  labels[0] = "Fly";
  ids[0] = kFreeCameraId;
  labels_cstr[0] = labels[0].c_str();

  int current_index = 0;
  // Add the enumerated cameras.
  for (unsigned i = 0; i < int_cast<unsigned>(cameras.size()); ++i)
  {
    const auto write_idx = i + 1;
    const auto &id = cameras[i];
    switch (id)
    {
    case handler::Camera::kRecordedCameraID:
      labels[write_idx] = "Recorded";
      break;
    default:
      labels[write_idx] = "Camera " + std::to_string(static_cast<int>(id));
      break;
    }
    labels_cstr[write_idx] = labels[write_idx].c_str();
    ids[write_idx] = static_cast<int>(id);

    if (id == active_camera_id)
    {
      current_index = static_cast<int>(write_idx);
    }
  }

  if (ImGui::Combo("Camera", &current_index, labels_cstr.data(),
                   static_cast<int>(labels_cstr.size())))
  {
    // Something.
    if (const auto set_camera_command = _set_camera_command.lock())
    {
      if (set_camera_command->admissible(viewer()))
      {
        set_camera_command->invoke(viewer(), ids[static_cast<unsigned>(current_index)]);
      }
    }
  }
}


void Hud::drawFps()
{
  const int fps_offset_y = 10;
  const int fps_width = 70;
  const int fps_height = 24;
  const ChildWindow child("FPSDisplay", { { { -fps_width, fps_offset_y }, Anchor::TopRight, true },
                                          { { fps_width, fps_height }, Stretch::None, true } });

  std::ostringstream out;
  out.precision(2);
  const auto fps = viewer().tes()->fps();
  out << std::fixed << std::setprecision(3) << fps << std::flush;

  const auto fps_str = out.str();
  ImGui::Text("%s", fps_str.c_str());
}
}  // namespace tes::view::ui
