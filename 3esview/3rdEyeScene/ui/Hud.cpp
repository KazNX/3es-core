//
// Author: Kazys Stepanas
//
#include "Hud.h"

#include <3escore/CoreUtil.h>
#include <3escore/Log.h>

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
  _window_flags |= ImGuiWindowFlags_NoBackground;
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
}


void Hud::drawCameraCombo()
{
  const auto &camera_handler = viewer().tes()->cameraHandler();
  using CameraId = handler::Camera::CameraId;
  std::vector<CameraId> cameras;
  const auto active_camera_id = viewer().activeCamera();
  camera_handler.enumerate(cameras);
  cameras.insert(cameras.begin(), handler::Camera::kInvalidCameraId);

  std::vector<std::string> labels(cameras.size());
  std::vector<CameraId> ids(cameras.size());
  std::vector<const char *> labels_cstr(labels.size());
  int current_index = 0;
  for (int i = 0; i < int_cast<int>(cameras.size()); ++i)
  {
    const auto &id = cameras[i];
    if (id != handler::Camera::kInvalidCameraId)
    {
      labels[i] = "Camera " + std::to_string(static_cast<int>(id));
    }
    else
    {
      labels[i] = "Default";
    }
    labels_cstr[i] = labels[i].c_str();
    ids[i] = id;

    if (id == active_camera_id)
    {
      current_index = i;
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
        set_camera_command->invoke(viewer(), ids[current_index]);
      }
    }
  }
}
}  // namespace tes::view::ui
