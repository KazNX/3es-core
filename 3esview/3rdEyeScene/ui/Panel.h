//
// Author: Kazys Stepanas
//
#ifndef TRD_EYE_SCENE_UI_PANEL_H
#define TRD_EYE_SCENE_UI_PANEL_H

#include <3rdEyeScene/ClientConfig.h>

#include "ImGui.h"

#include <3esview/util/Enum.h>

#include <Magnum/GL/Texture.h>

#include <functional>
#include <memory>

namespace tes::view::command
{
class Command;
}

namespace Magnum::ImGuiIntegration
{
class Context;
}  // namespace Magnum::ImGuiIntegration

namespace tes::view
{
class Viewer;
}  // namespace tes::view

namespace tes::view::ui
{
/// Base class for a UI panel - anything which draws using the immediate mode UI.
class TES_VIEWER_API Panel
{
public:
  enum class Anchor : unsigned
  {
    TopLeft,
    TopRight,
    BottomLeft,
    BottomRight,
    Centre,
    TopCentre,
    BottomCentre,
    CentreLeft,
    CentreRight,
  };

  enum class Stretch : unsigned
  {
    None = 0,
    Horizontal = (1 << 0),
    Vertical = (1 << 1)
  };

  struct PreferredCoordinates
  {
    struct Position
    {
      Magnum::Vector2i coord = {};
      Anchor anchor = Anchor::TopLeft;
      bool in_use = false;

      Magnum::Vector2i forView(const Magnum::Vector2i &viewport_size) const;
      ImVec2 forView(const ImVec2 &viewport_size) const;
    };
    struct Size
    {
      Magnum::Vector2i extents = {};
      Stretch stretch = Stretch::None;
      bool in_use = false;

      Magnum::Vector2i forView(const Magnum::Vector2i &viewport_size) const;
      ImVec2 forView(const ImVec2 &viewport_size) const;
    };

    Position position = {};
    Size size = {};
  };

  Panel(const std::string &name, Viewer &viewer,
        const PreferredCoordinates &preferred_coordinates = {})
    : _name(name)
    , _viewer(viewer)
    , _preferred_coordinates(preferred_coordinates)
  {}

  void setPreferredCoordinates(const PreferredCoordinates &preferred)
  {
    _preferred_coordinates = preferred;
  }
  void clearPreferredCoordinates() { setPreferredCoordinates({}); }
  const PreferredCoordinates &preferredCoordinates() const { return _preferred_coordinates; }

  /// Draw the pannel.
  ///
  /// This first calls @c defineWindow() before calling @c drawContent() which must be implemented
  /// by derived types.
  ///
  /// @param ui The UI context.
  void draw(Magnum::ImGuiIntegration::Context &ui);

  /// Get the size of the viewport used to draw the UI. This may differ from the window size.
  /// @return The UI viewport size.
  Magnum::Vector2i uiViewportSize() const;

  const std::string &name() const { return _name; }

  Viewer &viewer() { return _viewer; }
  const Viewer &viewer() const { return _viewer; }

protected:
  /// Result from @c button() function.
  enum ButtonResult
  {
    /// Button is inactive. Rendered if @c allow_inactive was passed true.
    ///
    /// A button can only be inactive when @c ButtonParam::command is not null and is inadmissible.
    Inactive,
    /// Button was drawn, but not pressed.
    Ok,
    /// Button was pressed. The @c ButtonParams::command will have been invoked if not null.
    Pressed,
  };

  /// Button parameterisation.
  struct ButtonParams
  {
    /// Button icon (if any).
    Magnum::GL::Texture2D *icon = nullptr;
    /// Button label (required).
    std::string label;
    /// Command to execute when pressed (if any).
    tes::view::command::Command *command = nullptr;
    /// Explicit drawing size
    ImVec2 size = { 0, 0 };

    ButtonParams() = default;
    ButtonParams(Magnum::GL::Texture2D *icon, std::string label,
                 tes::view::command::Command *command = nullptr)
      : icon(std::move(icon))
      , label(std::move(label))
      , command(command)
    {}
    ButtonParams(Magnum::GL::Texture2D *icon, std::string label,
                 tes::view::command::Command *command, const ImVec2 &size)
      : icon(std::move(icon))
      , label(std::move(label))
      , command(command)
      , size(size)
    {}
    ButtonParams(const ButtonParams &other) = default;
    ButtonParams(ButtonParams &&other) noexcept = default;

    ButtonParams &operator=(const ButtonParams &other) = default;
    ButtonParams &operator=(ButtonParams &&other) noexcept = default;
  };

  struct WindowBlock
  {
    void end()
    {
      if (_end)
      {
        _end();
      }
      _end = {};
    }

  protected:
    WindowBlock(std::function<void()> end)
      : _end(std::move(end))
    {}

    WindowBlock(WindowBlock &&other) noexcept
      : _end(std::exchange(other._end, {}))
    {}

    WindowBlock(const WindowBlock &) = delete;

    WindowBlock &operator=(WindowBlock &&other) noexcept
    {
      _end = std::exchange(other._end, _end);
      return *this;
    }

    WindowBlock &operator=(const WindowBlock &) = delete;

    ~WindowBlock() { end(); }

    std::function<void()> _end;
  };

  struct Window : public WindowBlock
  {
    static constexpr int kDefaultFlags =
      ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
    PreferredCoordinates coords = {};

    Window(const std::string &name, const Magnum::Vector2i &viewport_size,
           const PreferredCoordinates &coords, int window_flags = kDefaultFlags)
      : WindowBlock(makeEnd())
      , coords(coords)
    {
      if (coords.position.in_use)
      {
        const auto pos = coords.position.forView(viewport_size);
        ImGui::SetNextWindowPos({ static_cast<float>(pos.x()), static_cast<float>(pos.y()) });
      }
      if (coords.size.in_use)
      {
        const auto size = coords.size.forView(viewport_size);
        ImGui::SetNextWindowSize({ static_cast<float>(size.x()), static_cast<float>(size.y()) });
      }
      ImGui::Begin(name.c_str(), nullptr, window_flags);
    }

    Window(Window &&other) noexcept
      : WindowBlock(std::move(other))
      , coords(std::exchange(other.coords, {}))
    {}
    Window(const Window &other) = delete;

    ~Window() = default;

    Window &operator=(Window &&other) noexcept
    {
      WindowBlock::operator=(std::move(other));
      coords = std::exchange(other.coords, coords);
      return *this;
    }
    Window &operator=(const Window &other) = delete;

  protected:
    static std::function<void()> makeEnd()
    {
      return []() { ImGui::End(); };
    }
  };

  struct ChildWindow : public WindowBlock
  {
    PreferredCoordinates coords = {};

    ChildWindow(const std::string &name, const PreferredCoordinates &coords)
      : WindowBlock(makeEnd())
      , coords(coords)
    {
      const auto window_size = ImGui::GetWindowSize();
      if (coords.position.in_use)
      {
        ImGui::SetCursorPos(coords.position.forView(window_size));
      }
      const auto size = coords.size.forView(window_size);
      ImGui::BeginChild(name.c_str(), size);
    }

    ChildWindow(ChildWindow &&other) noexcept
      : WindowBlock(std::move(other))
      , coords(std::exchange(other.coords, {}))
    {}

    ChildWindow(const ChildWindow &other) = delete;

    ~ChildWindow() = default;

    ChildWindow &operator=(ChildWindow &&other) noexcept
    {
      WindowBlock::operator=(std::move(other));
      coords = std::exchange(other.coords, coords);
      return *this;
    }
    ChildWindow &operator=(const ChildWindow &other) = delete;

  protected:
    static std::function<void()> makeEnd()
    {
      return []() { ImGui::EndChild(); };
    }
  };

  /// Begin a new window and define it's size and position.
  ///
  /// This returns a @c Window object which should be kept on the stack and ends the window
  /// defintion when disposed of.
  /// @param preferred_coordinates
  /// @param flags ImGUI window flags.
  /// @return
  virtual Window defineWindow(const PreferredCoordinates &preferred_coordinates, int flags);

  /// Draws the content for the panel
  /// @param ui
  virtual void drawContent(Magnum::ImGuiIntegration::Context &ui, Window &window) = 0;

  /// Draw a button associated with the given action.
  /// @param params Details of the button.
  /// @param allow_inactive When true, draws the action icon as inactive, otherwise draws nothing.
  ButtonResult button(const ButtonParams &params, bool allow_inactive = true);

  std::string _name;
  int _window_flags = Window::kDefaultFlags;
  Viewer &_viewer;
  PreferredCoordinates _preferred_coordinates = {};
};
}  // namespace tes::view::ui

TES_ENUM_FLAGS(tes::view::ui::Panel::Stretch, unsigned);

#endif  // TRD_EYE_SCENE_UI_PANEL_H
