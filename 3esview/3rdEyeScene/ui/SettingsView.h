//
// Author: Kazys Stepanas
//
#ifndef TRD_EYE_SCENE_UI_SETTINGS_VIEW_H
#define TRD_EYE_SCENE_UI_SETTINGS_VIEW_H

#include <3rdEyeScene/ClientConfig.h>

#include "TreeView.h"

#include <3esview/settings/Settings.h>

#include <Magnum/GL/Texture.h>

#include <array>
#include <memory>

namespace tes::view::data
{
class DataThread;
}  // namespace tes::view::data

namespace tes::view::command
{
class Command;
}  // namespace tes::view::command

namespace tes::view::ui
{
class TES_VIEWER_API SettingsView : public TreeView
{
public:
  SettingsView(Viewer &viewer);

private:
  void drawContent(Magnum::ImGuiIntegration::Context &ui, Window &window) override;

  bool show(unsigned idx, settings::Camera &config);
  bool show(unsigned idx, settings::Log &config);
  bool show(unsigned idx, settings::Playback &config);
  bool show(unsigned idx, settings::Render &config);
  bool show(unsigned idx, settings::Extension &config);

  bool showProperty(unsigned idx, settings::Bool &prop);
  bool showProperty(unsigned idx, settings::Int &prop);
  bool showProperty(unsigned idx, settings::UInt &prop);
  bool showProperty(unsigned idx, settings::Float &prop);
  bool showProperty(unsigned idx, settings::Double &prop);
  bool showProperty(unsigned idx, settings::Colour &prop);
  bool showProperty(unsigned idx, settings::Enum &prop);
};
}  // namespace tes::view::ui

#endif  // TRD_EYE_SCENE_UI_SETTINGS_VIEW_H
