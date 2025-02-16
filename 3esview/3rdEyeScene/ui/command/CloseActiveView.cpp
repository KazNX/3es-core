#include "CloseActiveView.h"

#include "UIViewer.h"
#include "ui/IconBar.h"

namespace tes::view::ui::command
{
CloseActiveView::CloseActiveView()
  : view::command::Command("close active view", view::command::Args())
{}


bool CloseActiveView::checkAdmissible(Viewer &viewer) const
{
  auto *ui_viewer = dynamic_cast<UIViewer *>(&viewer);
  return ui_viewer && ui_viewer->uiEnabled() &&
         ui_viewer->iconBar().activeView() != ui::IconBar::View::Invalid;
  return false;
}


view::command::CommandResult CloseActiveView::invoke(
  Viewer &viewer, [[maybe_unused]] const ExecInfo &info,
  [[maybe_unused]] const view::command::Args &args)
{
  auto *ui_viewer = dynamic_cast<UIViewer *>(&viewer);
  if (!ui_viewer)
  {
    return { view::command::CommandResult::Code::Failed, "No active view." };
  }

  ui_viewer->iconBar().closeActiveView();
  return { view::command::CommandResult::Code::Ok };
}
}  // namespace tes::view::ui::command
