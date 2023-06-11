//
// Author: Kazys Stepanas
//
#include "LogView.h"

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
}
}  // namespace tes::view::ui
