//
// Author: Kazys Stepanas
//
#include "UIViewer.h"

#include "ui/CategoriesView.h"
#include "ui/ConnectView.h"
#include "ui/Hud.h"
#include "ui/IconBar.h"
#include "ui/LogView.h"
#include "ui/Playback.h"
#include "ui/SettingsView.h"
#include "ui/command/ToggleCategories.h"
#include "ui/command/ToggleConnect.h"
#include "ui/command/ToggleLog.h"
#include "ui/command/ToggleSettings.h"

#include <3esview/command/Set.h>
#include <3esview/ThirdEyeScene.h>
#include <3esview/settings/Settings.h>

namespace tes::view
{
namespace
{
constexpr const char *kWindowSettingsName = "Window";
constexpr const char *kWindowSettingsHorizontal = "Horizontal";
constexpr const char *kWindowSettingsVertical = "Vertical";
using WindowSizeProperty = settings::UInt;

std::vector<settings::Extension> buildExtendedSettings()
{
  std::vector<settings::Extension> extensions;
  extensions.emplace_back(settings::Extension{ kWindowSettingsName });
  auto &window_settings = extensions.back();
  window_settings.add(settings::ExtensionProperty{ WindowSizeProperty{
    kWindowSettingsHorizontal, 800u, 0u, 10000u, "Horizontal pixel resolution." } });
  window_settings.add(settings::ExtensionProperty{ WindowSizeProperty{
    kWindowSettingsVertical, 600u, 0u, 10000u, "Vertical pixel resolution." } });
  return extensions;
}
}  // namespace

class ToggleUI : public command::Command
{
public:
  ToggleUI()
    : command::Command("toggleUI", command::Args())
  {}

protected:
  bool checkAdmissible(Viewer &viewer) const
  {
    auto *ui_viewer = dynamic_cast<UIViewer *>(&viewer);
    return ui_viewer != nullptr;
  }

  command::CommandResult invoke(Viewer &viewer, const ExecInfo &info,
                                const command::Args &args) override
  {
    TES_UNUSED(info);
    TES_UNUSED(args);
    auto *ui_viewer = dynamic_cast<UIViewer *>(&viewer);
    if (!ui_viewer)
    {
      return { command::CommandResult::Code::Invalid };
    }

    ui_viewer->setUiEnabled(!ui_viewer->uiEnabled());

    return { command::CommandResult::Code::Ok };
  }
};


UIViewer::GuiContext::GuiContext(ImGuiContext *context)
  : _current(context)
{
  _restore = ImGui::GetCurrentContext();
  ImGui::SetCurrentContext(context);
}


UIViewer::GuiContext::~GuiContext()
{
  ImGui::SetCurrentContext(_restore);
}


UIViewer::UIViewer(const Arguments &arguments)
  : Viewer(arguments, buildExtendedSettings())
{
  _imgui = Magnum::ImGuiIntegration::Context(Magnum::Vector2{ windowSize() } / dpiScaling(),
                                             windowSize(), framebufferSize());
  initialiseUi();
  commands()->registerCommand(std::make_shared<ToggleUI>(), command::Shortcut("F2"));
  const auto config = tes()->settings().config();
  // Settings will have been restored at this point. Set the window size.
  updateWindowSize(config);
  tes()->settings().addObserver(
    [this](const settings::Settings::Config &config) { updateWindowSize(config); });
}


UIViewer::~UIViewer() = default;


UIViewer::DrawMode UIViewer::onDrawStart(float dt)
{
  TES_UNUSED(dt);
  const GuiContext gui_context(_imgui.context());

  if (ImGui::GetIO().WantTextInput && !isTextInputActive())
  {
    startTextInput();
  }
  else if (!ImGui::GetIO().WantTextInput && isTextInputActive())
  {
    stopTextInput();
  }

  return isTextInputActive() ? DrawMode::Modal : DrawMode::Normal;
}


void UIViewer::drawEvent()
{
  Viewer::drawEvent();
  const auto window_size = windowSize();
  if (_expected_window_size != window_size)
  {
    // Copy the window size as we can end up modifying it again.
    // Hack: adjust for dpiScaling() const windowSize() doesn't consider it, but setWindowSize()
    // does.
    const auto new_size =
      (Magnum::Vector2(_expected_window_size) / dpiScaling()) + Magnum::Vector2(0.5f, 0.5f);
    setWindowSize(Magnum::Vector2i(new_size));
  }
}


void UIViewer::onDrawComplete(float dt)
{
  TES_UNUSED(dt);

  const GuiContext gui_context(_imgui.context());

  _imgui.newFrame();

  if (_ui_enabled)
  {
    for (const auto &panel : _panels)
    {
      panel->draw(_imgui);
    }

    _imgui.updateApplicationCursor(*this);
  }

  // Set render state for the UI
  Magnum::GL::Renderer::setBlendEquation(Magnum::GL::Renderer::BlendEquation::Add,
                                         Magnum::GL::Renderer::BlendEquation::Add);
  Magnum::GL::Renderer::setBlendFunction(Magnum::GL::Renderer::BlendFunction::SourceAlpha,
                                         Magnum::GL::Renderer::BlendFunction::OneMinusSourceAlpha);

  Magnum::GL::Renderer::enable(Magnum::GL::Renderer::Feature::Blending);
  Magnum::GL::Renderer::enable(Magnum::GL::Renderer::Feature::ScissorTest);
  Magnum::GL::Renderer::disable(Magnum::GL::Renderer::Feature::FaceCulling);
  Magnum::GL::Renderer::disable(Magnum::GL::Renderer::Feature::DepthTest);

  _imgui.drawFrame();

  // Clear UI render states
  Magnum::GL::Renderer::enable(Magnum::GL::Renderer::Feature::DepthTest);
  Magnum::GL::Renderer::enable(Magnum::GL::Renderer::Feature::FaceCulling);
  Magnum::GL::Renderer::disable(Magnum::GL::Renderer::Feature::ScissorTest);
  Magnum::GL::Renderer::disable(Magnum::GL::Renderer::Feature::Blending);

  Magnum::GL::Renderer::setBlendEquation(Magnum::GL::Renderer::BlendEquation::Add,
                                         Magnum::GL::Renderer::BlendEquation::Add);
  Magnum::GL::Renderer::setBlendFunction(Magnum::GL::Renderer::BlendFunction::One,
                                         Magnum::GL::Renderer::BlendFunction::Zero);
}


void UIViewer::viewportEvent(ViewportEvent &event)
{
  Viewer::viewportEvent(event);
  _imgui.relayout(Magnum::Vector2{ event.windowSize() } / event.dpiScaling(), event.windowSize(),
                  event.framebufferSize());
  // Update settings if required.
  if (event.windowSize() != _expected_window_size)
  {
    _expected_window_size = event.windowSize();
    auto config = tes()->settings().config();
    for (auto iter = config.extentions.begin(); iter != config.extentions.end(); ++iter)
    {
      if (iter->name() == kWindowSettingsName)
      {
        auto *hsize = (*iter)[kWindowSettingsHorizontal].getProperty<WindowSizeProperty>();
        auto *vsize = (*iter)[kWindowSettingsVertical].getProperty<WindowSizeProperty>();

        if (hsize->value() != _expected_window_size.x() ||
            vsize->value() != _expected_window_size.y())
        {
          hsize->setValue(_expected_window_size.x());
          vsize->setValue(_expected_window_size.y());
          tes()->settings().update(config);
        }
        break;
      }
    }
  }
}


void UIViewer::initialiseUi()
{
  initialiseImGui();
  initialisePlaybackUi();
  initialiseIconBarUi();
  initialiseHud();
}


void UIViewer::initialiseImGui()
{
  // The ImGuiIntegration::Context initialises the KeyMap incorrectly. It is meant to index items
  // in KeysDown, and must be in range [-1, 511] with -1 used for unmapped keys.
  // TODO(KS): this may break some UI functionality, so I'll need to work out what the mappings
  // should be. The Magnum code actually flags the mappings as suspect.
  const GuiContext gui_context(_imgui.context());
  ImGuiIO &io = ImGui::GetIO();
  for (auto &key : io.KeyMap)
  {
    if (key != -1)
    {
      key = -1;
    }
  }
}


void UIViewer::initialiseHud()
{
  // Position the panel by stretching the available space, removing the icon bar and playback bar
  // sizes.
  const ui::Panel::PreferredCoordinates coords = {
    { { ui::IconBar::kPanelSize, 0 }, ui::Panel::Anchor::TopLeft, true },
    { { -ui::IconBar::kPanelSize, -ui::Playback::kPanelSize },
      ui::Panel::Stretch::Horizontal | ui::Panel::Stretch::Vertical,
      true }
  };
  const auto hud = std::make_shared<ui::Hud>(*this, coords);
  _panels.emplace_back(hud);
}


void UIViewer::initialiseIconBarUi()
{
  auto icon_bar = std::make_shared<ui::IconBar>(*this);
  auto command_set = this->commands();

  std::shared_ptr<command::Command> command;

  command = std::make_shared<ui::command::ToggleCategories>(*icon_bar);
  command_set->registerCommand(command);
  icon_bar->registerCommand(ui::IconBar::View::Categories, command);
  icon_bar->registerView(ui::IconBar::View::Categories,
                         std::make_shared<ui::CategoriesView>(*this));

  command = std::make_shared<ui::command::ToggleConnect>(*icon_bar);
  command_set->registerCommand(command);
  icon_bar->registerCommand(ui::IconBar::View::Connect, command);
  icon_bar->registerView(ui::IconBar::View::Connect, std::make_shared<ui::ConnectView>(*this));

  command = std::make_shared<ui::command::ToggleLog>(*icon_bar);
  command_set->registerCommand(command);
  icon_bar->registerCommand(ui::IconBar::View::Log, command);
  icon_bar->registerView(ui::IconBar::View::Log, std::make_shared<ui::LogView>(*this));

  command = std::make_shared<ui::command::ToggleSettings>(*icon_bar);
  command_set->registerCommand(command);
  icon_bar->registerCommand(ui::IconBar::View::Settings, command);
  icon_bar->registerView(ui::IconBar::View::Settings, std::make_shared<ui::SettingsView>(*this));

  _panels.emplace_back(icon_bar);
}


void UIViewer::initialisePlaybackUi()
{
  auto playback = std::make_shared<ui::Playback>(*this);
  _panels.emplace_back(playback);
}


void UIViewer::mousePressEvent(MouseEvent &event)
{
  if (_imgui.handleMousePressEvent(event))
  {
    return;
  }
  Viewer::mousePressEvent(event);
}


void UIViewer::mouseReleaseEvent(MouseEvent &event)
{
  if (_imgui.handleMouseReleaseEvent(event))
  {
    return;
  }
  Viewer::mouseReleaseEvent(event);
}


void UIViewer::mouseMoveEvent(MouseMoveEvent &event)
{
  if (_imgui.handleMouseMoveEvent(event))
  {
    return;
  }
  Viewer::mouseMoveEvent(event);
}


void UIViewer::mouseScrollEvent(MouseScrollEvent &event)
{
  if (_imgui.handleMouseScrollEvent(event))
  {
    // Prevent scrolling the page
    event.setAccepted();
    return;
  }
  // Viewer::mouseScrollEvent(event);
}


void UIViewer::keyPressEvent(KeyEvent &event)
{
  if (_imgui.handleKeyPressEvent(event))
  {
    return;
  }
  Viewer::keyPressEvent(event);
}


void UIViewer::keyReleaseEvent(KeyEvent &event)
{
  if (_imgui.handleKeyReleaseEvent(event))
  {
    return;
  }
  Viewer::keyReleaseEvent(event);
}


void UIViewer::textInputEvent(TextInputEvent &event)
{
  if (_imgui.handleTextInputEvent(event))
  {
    return;
  }
  // Viewer::textInputEvent(event);
}


void UIViewer::updateWindowSize(const settings::Settings::Config &config)
{
  for (auto iter = config.extentions.begin(); iter != config.extentions.end(); ++iter)
  {
    if (iter->name() == kWindowSettingsName)
    {
      const auto &window_settings = *iter;
      _expected_window_size = Magnum::Vector2i(
        window_settings[kWindowSettingsHorizontal].getProperty<WindowSizeProperty>()->value(),
        window_settings[kWindowSettingsVertical].getProperty<WindowSizeProperty>()->value());
      break;
    }
  }
}
}  // namespace tes::view
