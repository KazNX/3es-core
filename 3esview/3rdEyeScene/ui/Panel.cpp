//
// Author: Kazys Stepanas
//
#include "Panel.h"

#include <3esview/command/Command.h>
#include <3esview/Viewer.h>

#include <Magnum/ImGuiIntegration/Context.hpp>

namespace tes::view::ui
{
Magnum::Vector2i Panel::PreferredCoordinates::Position::forView(
  const Magnum::Vector2i &viewport_size) const
{
  if (!in_use)
  {
    return { 0, 0 };
  }

  auto pos = coord;
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

  return pos;
}


Magnum::Vector2i Panel::PreferredCoordinates::Size::forView(
  const Magnum::Vector2i &viewport_size) const
{
  if (!in_use)
  {
    return { 0, 0 };
  }

  auto size = extents;
  if ((stretch & Stretch::Horizontal) != Stretch::None)
  {
    size.x() += viewport_size.x();
  }
  if ((stretch & Stretch::Vertical) != Stretch::None)
  {
    size.y() += viewport_size.y();
  }

  return size;
}


void Panel::draw(Magnum::ImGuiIntegration::Context &ui)
{
  auto window = defineWindow(preferredCoordinates());
  drawContent(ui, window);
}


Magnum::Vector2i Panel::uiViewportSize() const
{
  return _viewer.windowSize() / _viewer.dpiScaling();
}


Panel::Window Panel::defineWindow(const PreferredCoordinates &preferred_coordinates)
{
  return { name(), uiViewportSize(), preferred_coordinates };
}


Panel::ButtonResult Panel::button(const ButtonParams &params, bool allow_inactive)
{
  const bool active = !params.command || params.command->admissible(_viewer);
  if (active)
  {
    // Try for an image first.
    bool pressed = false;

    if (params.icon && params.icon->id())
    {
      pressed = ImGui::ImageButton(params.label.c_str(), params.icon, params.size);
    }
    else
    {
      pressed = ImGui::Button(params.label.c_str(), params.size);
    }

    if (pressed)
    {
      if (params.command)
      {
        params.command->invoke(_viewer);
      }
      return ButtonResult::Pressed;
    }
    return ButtonResult::Ok;
  }

  // Draw inactive.
  if (allow_inactive)
  {
    if (params.icon && params.icon->id())
    {
      // Padding to make sure the images rendering the same size as the ImageButton equivalents.
      // Determined empirically.
      constexpr static int kDisableButtonPaddingX = 6;
      constexpr static int kDisableButtonPaddingY = 4;
      ImGui::Image(params.icon, ImVec2{ params.size.x + kDisableButtonPaddingX,
                                        params.size.y + kDisableButtonPaddingY });
    }
    else
    {
      ImGui::Text(params.label.c_str());
    }
  }
  return ButtonResult::Inactive;
}
}  // namespace tes::view::ui
