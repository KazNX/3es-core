//
// Author: Kazys Stepanas
//
#ifndef TRD_EYE_SCENE_UI_LOG_VIEW_H
#define TRD_EYE_SCENE_UI_LOG_VIEW_H

#include <3rdEyeScene/ClientConfig.h>

#include "Panel.h"

#include <3esview/ViewerLog.h>

#include <3escore/Log.h>

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
class LogView : public Panel
{
public:
  static constexpr size_t kBufferStep = 100 * 1024;
  static constexpr int kToolbarHeight = 16;
  LogView(Viewer &viewer);

private:
  struct ToolState
  {
    enum class Action
    {
      None,
      JumpStart,
      JumpEnd
    };

    Action action = Action::None;
    bool level_changed = false;
  };

  void drawContent(Magnum::ImGuiIntegration::Context &ui, Window &window) override;

  /// Draw the "toolbar" section - log filter and jump to buttons.
  ToolState drawTools();
  /// Draw the log text window.
  void drawText(const ToolState &tool_state);

  inline log::Level filterLogLevel() const
  {
    return _log_levels[static_cast<unsigned>(_log_level_filter_index)];
  }

  /// Log level combo box index. Start with no filter.
  int _log_level_filter_index = 4;
  float _last_content_width = 0;
  /// True if we are viewing the tail. When true, we keep the end of the log visible. This becomes
  /// false when we scroll up, and true when we scroll to the end.
  bool _view_tail = true;

  std::array<const char *, 5> _log_level_names = { "Fatal error", "Error", "Warning", "Info",
                                                   "Trace" };
  std::array<log::Level, 5> _log_levels = { log::Level::Fatal, log::Level::Error, log::Level::Warn,
                                            log::Level::Info, log::Level::Trace };
};
}  // namespace tes::view::ui

#endif  // TRD_EYE_SCENE_UI_LOG_VIEW_H
