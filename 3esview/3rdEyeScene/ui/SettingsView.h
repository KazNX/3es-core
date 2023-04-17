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

  struct ViewStatus
  {
    bool active = false;
    bool dirty = false;

    ViewStatus &operator+=(const ViewStatus &other)
    {
      active = active + other.active;
      dirty = dirty + other.dirty;
      return *this;
    }
  };

  ViewStatus show(unsigned idx, settings::Camera &config);
  ViewStatus show(unsigned idx, settings::Log &config);
  ViewStatus show(unsigned idx, settings::Playback &config);
  ViewStatus show(unsigned idx, settings::Render &config);
  ViewStatus show(unsigned idx, settings::Extension &config);

  ViewStatus showProperty(unsigned idx, settings::Bool &prop);
  ViewStatus showProperty(unsigned idx, settings::Int &prop);
  ViewStatus showProperty(unsigned idx, settings::UInt &prop);
  ViewStatus showProperty(unsigned idx, settings::Float &prop);
  ViewStatus showProperty(unsigned idx, settings::Double &prop);
  ViewStatus showProperty(unsigned idx, settings::Colour &prop);
  ViewStatus showProperty(unsigned idx, settings::Enum &prop);

  settings::Settings::Config _cached_config;
  bool _editing_config = false;
};
}  // namespace tes::view::ui

#endif  // TRD_EYE_SCENE_UI_SETTINGS_VIEW_H
