//
// Author: Kazys Stepanas
//
#include "LogView.h"

#include <3esview/Viewer.h>

#include <3escore/CoreUtil.h>
#include <3escore/Log.h>

#include <Magnum/ImGuiIntegration/Context.hpp>

namespace tes::view::ui
{
LogView::LogView(Viewer &viewer)
  : Panel("Log", viewer)
{}


void LogView::drawContent(Magnum::ImGuiIntegration::Context &ui, Window &window)
{
  TES_UNUSED(ui);
  TES_UNUSED(window);
  const auto tool_state = drawTools();
  drawText(tool_state);
}


LogView::ToolState LogView::drawTools()
{
  ToolState state = {};
  ChildWindow child("LogTools", { {}, { { 0, kToolbarHeight }, Stretch::Horizontal, true } });

  if (ImGui::Button("Jump to Start"))
  {
    state.action = ToolState::Action::JumpStart;
  }
  ImGui::SameLine();
  if (ImGui::Button("Jump to End"))
  {
    state.action = ToolState::Action::JumpEnd;
  }
  ImGui::SameLine();

  const auto last_filter_index = _log_level_filter_index;
  if (ImGui::Combo("LogLevel", &_log_level_filter_index, _log_level_names.data(),
                   int_cast<int>(_log_level_names.size())))
  {
    state.level_changed = last_filter_index != _log_level_filter_index;
  }

  ImGui::SameLine();
  return state;
}


void LogView::drawText(const ToolState &tool_state)
{
  // Get the height of one line of text.
  const auto line_height = ImGui::GetTextLineHeightWithSpacing();
  auto log_view = viewer().logger().view(filterLogLevel());
  // Work out how many lines we can display.
  const auto lines_available = static_cast<float>(log_view.size());
  const auto scroll_size_y = line_height * lines_available;
  ImGui::SetNextWindowContentSize({ _last_content_width, scroll_size_y });
  ChildWindow child("LogView", {});
  // Get the available space.
  const auto available_height = ImGui::GetWindowHeight();
  const auto displayable_lines = static_cast<size_t>(std::ceil(available_height / line_height));

  switch (tool_state.action)
  {
  default:
  case ToolState::Action::None:
    break;
  case ToolState::Action::JumpStart:
    ImGui::SetScrollY(0.0f);
    _view_tail = false;
    break;
  case ToolState::Action::JumpEnd:
    _view_tail = true;
    break;
  }

  if (_view_tail)
  {
    ImGui::SetScrollY(std::max(scroll_size_y - static_cast<float>(displayable_lines), 0.0f));
  }

  // TODO(KS): filter the log by the selected log level. This completely changes how we render as
  // we have fewer items to display and need to adjust the content view accordingly.
  // Once idea is to request a filtered view of all the items, and amortise building
  // that and only render new text once that completes.
  ImGuiListClipper clipper;
  clipper.Begin(static_cast<int>(lines_available), line_height);

  while (clipper.Step())
  {
    auto iter = log_view.begin() + static_cast<unsigned>(clipper.DisplayStart);
    const auto end = log_view.end();
    for (int line_number = clipper.DisplayStart; line_number < clipper.DisplayEnd && iter != end;
         ++iter, ++line_number)
    {
      const auto entry = *iter;
      const auto text_size = ImGui::CalcTextSize(entry.message.c_str());
      _last_content_width = text_size.x;
      ImGui::TextUnformatted(entry.message.c_str());
    }
  }

  clipper.End();

  _view_tail = tool_state.action != ToolState::Action::JumpStart &&
               ImGui::GetScrollY() == ImGui::GetScrollMaxY();

  log_view.release();

#if 0
  // Test log window scrolling code.
  static log::Level level = log::Level::Trace;

  switch (level)
  {
  case log::Level::Fatal:
    log::fatal(level, "Scroll @ ", ImGui::GetScrollY(), "/", ImGui::GetScrollMaxY(),
               " of content: ", scroll_size_y);
    level = log::Level::Trace;
    break;
  case log::Level::Error:
    log::error(level, "Scroll @ ", ImGui::GetScrollY(), "/", ImGui::GetScrollMaxY(),
               " of content: ", scroll_size_y);
    level = log::Level::Trace;  // Don't do fatal. That raises an exception.
    break;
  case log::Level::Warn:
    log::warn(level, "Scroll @ ", ImGui::GetScrollY(), "/", ImGui::GetScrollMaxY(),
              " of content: ", scroll_size_y);
    level = log::Level::Error;
    break;
  default:
  case log::Level::Info:
    log::info(level, "Scroll @ ", ImGui::GetScrollY(), "/", ImGui::GetScrollMaxY(),
              " of content: ", scroll_size_y);
    level = log::Level::Warn;
    break;
  case log::Level::Trace:
    log::trace(level, "Scroll @ ", ImGui::GetScrollY(), "/", ImGui::GetScrollMaxY(),
               " of content: ", scroll_size_y);
    level = log::Level::Info;
    break;
  }
#endif  // #
}
}  // namespace tes::view::ui
