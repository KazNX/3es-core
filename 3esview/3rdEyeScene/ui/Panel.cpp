//
// Author: Kazys Stepanas
//
#include "Panel.h"

#include <3esview/Viewer.h>

#include <Magnum/ImGuiIntegration/Context.hpp>

namespace tes::view::ui
{
Magnum::Vector2i Panel::uiViewportSize() const
{
  return _viewer.windowSize() / _viewer.dpiScaling();
}


void Panel::setNextWindowPos(Magnum::Vector2i pos, Anchor anchor) const
{
  const auto viewport_size = uiViewportSize();
  // Fix left/right adjustment
  switch (anchor)
  {
  default:
  case Anchor::TopLeft:
  case Anchor::BottomLeft:
  case Anchor::CentreLeft:
    // No x change
    break;
  case Anchor::TopRight:
  case Anchor::BottomRight:
  case Anchor::CentreRight:
    // Right relative.
    pos.x() += viewport_size.x();
    break;
  case Anchor::TopCentre:
  case Anchor::BottomCentre:
  case Anchor::Centre:
    // Centre relative.
    pos.x() += viewport_size.x() / 2;
    break;
  }

  // Fix top/bottom adjustment
  switch (anchor)
  {
  default:
  case Anchor::TopLeft:
  case Anchor::TopRight:
  case Anchor::TopCentre:
    // No y change
    break;
  case Anchor::BottomLeft:
  case Anchor::BottomRight:
  case Anchor::BottomCentre:
    // Bottom relative.
    pos.y() += viewport_size.y();
    break;
  case Anchor::CentreLeft:
  case Anchor::CentreRight:
  case Anchor::Centre:
    // Centre relative.
    pos.y() += viewport_size.y() / 2;
    break;
  }

  ImGui::SetNextWindowPos({ static_cast<float>(pos.x()), static_cast<float>(pos.y()) });
}


void Panel::setNextWindowSize(Magnum::Vector2i size, Stretch stretch) const
{
  const auto viewport_size = uiViewportSize();
  switch (stretch)
  {
  default:
  case Stretch::None:
    // No change
    break;
  case Stretch::Horizontal:
    size.x() += viewport_size.x();
    break;
  case Stretch::Vertical:
    size.y() += viewport_size.y();
    break;
  }

  ImGui::SetNextWindowSize({ static_cast<float>(size.x()), static_cast<float>(size.y()) });
}
}  // namespace tes::view::ui
