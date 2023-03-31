//
// Author: Kazys Stepanas
//
#ifndef TRD_EYE_SCENE_UI_HUD_H
#define TRD_EYE_SCENE_UI_HUD_H

#include <3rdEyeScene/ClientConfig.h>

#include "Panel.h"

#include <Magnum/GL/Texture.h>

#include <memory>
#include <utility>
#include <vector>

namespace tes::view::data
{
class DataThread;
}

namespace tes::view::command
{
class Command;
}

namespace tes::view::ui
{
class TES_VIEWER_API Hud : public Panel
{
public:
  constexpr static int kButtonSize = 24;
  constexpr static int kPanelSize = 3 * kButtonSize;

  Hud(Viewer &viewer);
  Hud(Viewer &viewer, const PreferredCoordinates &coords);

private:
  void drawContent(Magnum::ImGuiIntegration::Context &ui, Window &window);
  void drawCameraCombo();
  void drawFps();

  std::weak_ptr<command::Command> _set_camera_command;
};
}  // namespace tes::view::ui

#endif  // TRD_EYE_SCENE_UI_HUD_H
