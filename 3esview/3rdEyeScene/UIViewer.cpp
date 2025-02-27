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

// FIXME: Circular includes
#include "ui/command/CloseActiveView.h"
#include "ui/command/ToggleCategories.h"
#include "ui/command/ToggleConnect.h"
#include "ui/command/ToggleLog.h"
#include "ui/command/ToggleSettings.h"

#include <3esview/command/Set.h>
#include <3esview/ThirdEyeScene.h>
#include <3esview/settings/Settings.h>

#include <3escore/CoreUtil.h>
#include <3escore/Finally.h>

#include <cxxopts.hpp>

#include <iostream>

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

/// Command which toggles UI rendering.
///
/// See @c UIViewer::setUiEnabled()
class ToggleUI : public command::Command
{
public:
  ToggleUI()
    : command::Command("toggleUI", command::Args())
  {}

protected:
  bool checkAdmissible(Viewer &viewer) const override
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


void UICommandLineOptions::addOptions(cxxopts::Options &parser)
{
  Super::addOptions(parser);

  // clang-format off
  parser.add_options("Window")
    ("height", "Initial window width.", cxxopts::value(window.size.y()))
    ("width", "Initial window height.", cxxopts::value(window.size.x()))
    ;
  // clang-format on
}


bool UICommandLineOptions::validate(const cxxopts::ParseResult &parsed)
{
  if (!Super::validate(parsed))
  {
    return false;
  }

  window.use_height = parsed.count("height") > 0;
  window.use_width = parsed.count("width") > 0;

  if (window.use_width && window.size.x() <= 0)
  {
    std::cerr << "Invalid window width: " << window.size.x() << std::endl;
    return false;
  }

  if (window.use_height && window.size.y() <= 0)
  {
    std::cerr << "Invalid window height: " << window.size.y() << std::endl;
    return false;
  }

  return true;
}


UIViewer::GuiContext::GuiContext(ImGuiContext *context)
  : _current(context)
{
  _restore = ImGui::GetCurrentContext();
  ImGui::SetCurrentContext(context);
  // Disable imgui.ini saving.
  auto &imgui_io = ImGui::GetIO();
  imgui_io.IniFilename = nullptr;
  imgui_io.LogFilename = nullptr;
}


UIViewer::GuiContext::~GuiContext()
{
  ImGui::SetCurrentContext(_restore);
}


UIViewer::UIViewer(const UIViewArguments &arguments)
  : Viewer(arguments, buildExtendedSettings())
{
  _imgui = Magnum::ImGuiIntegration::Context(Magnum::Vector2{ windowSize() } / dpiScaling(),
                                             windowSize(), framebufferSize());
  initialiseUi();
  commands()->registerCommand(std::make_shared<ToggleUI>(), command::Shortcut("F8"));
  const auto config = tes()->settings().config();
  // Settings will have been restored at this point. Set the window size.
  updateWindowSize(config, dynamic_cast<const UICommandLineOptions *>(&commandLineOptions()));
  tes()->settings().addObserver(
    [this](const settings::Settings::Config &config) { updateWindowSize(config); });
}


UIViewer::~UIViewer() = default;


UIViewer::DrawMode UIViewer::onDrawStart(float dt)
{
  TES_UNUSED(dt);
  _in_startup = false;
  prepareUiRender();
  return isTextInputActive() ? DrawMode::Modal : DrawMode::Normal;
}


void UIViewer::setWindowSize(const Magnum::Vector2i &size)
{
  Viewer::setWindowSize(size);
}


void UIViewer::onDrawComplete(float dt)
{
  TES_UNUSED(dt);
  completeUiRender();
  if (_pending_window_size)
  {
    const auto window_size = *_pending_window_size;
    _pending_window_size.reset();
    setWindowSize(window_size);
  }
}


void UIViewer::viewportEvent(ViewportEvent &event)
{
  // Flag we are in a viewport event so that settings handle won't resize the window.
  _in_viewport_event = true;
  const auto at_exit = finally([this]() { _in_viewport_event = false; });

  Viewer::viewportEvent(event);
  relayoutUi();

  // Update settings if not coming from a settings event.
  if (!_in_settings_notify)
  {
    const auto window_size = event.windowSize();
    auto config = tes()->settings().config();
    for (auto iter = config.extentions.begin(); iter != config.extentions.end(); ++iter)
    {
      if (iter->name() == kWindowSettingsName)
      {
        auto *hsize = (*iter)[kWindowSettingsHorizontal].getProperty<WindowSizeProperty>();
        auto *vsize = (*iter)[kWindowSettingsVertical].getProperty<WindowSizeProperty>();

        if (static_cast<int>(hsize->value()) != window_size.x() ||
            static_cast<int>(vsize->value()) != window_size.y())
        {
          hsize->setValue(int_cast<unsigned>(window_size.x()));
          vsize->setValue(int_cast<unsigned>(window_size.y()));
          tes()->settings().update(config);
        }
        break;
      }
    }
  }
}


void UIViewer::relayoutUi()
{
  _imgui.relayout(Magnum::Vector2{ windowSize() } / dpiScaling(), windowSize(), framebufferSize());
}

void UIViewer::prepareUiRender()
{
  const GuiContext gui_context(_imgui.context());

  if (ImGui::GetIO().WantTextInput && !isTextInputActive())
  {
    startTextInput();
  }
  else if (!ImGui::GetIO().WantTextInput && isTextInputActive())
  {
    stopTextInput();
  }
}


void UIViewer::completeUiRender()
{
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
  _icon_bar = std::make_shared<ui::IconBar>(*this);
  auto command_set = this->commands();

  std::shared_ptr<command::Command> command;

  command = std::make_shared<ui::command::ToggleConnect>(*_icon_bar);
  command_set->registerCommand(command, command::Shortcut("f2"));
  command_set->registerCommand(command, command::Shortcut("ctrl+k"));
  _icon_bar->registerCommand(ui::IconBar::View::Connect, command);
  _icon_bar->registerView(ui::IconBar::View::Connect, std::make_shared<ui::ConnectView>(*this));

  command = std::make_shared<ui::command::ToggleCategories>(*_icon_bar);
  command_set->registerCommand(command, command::Shortcut("f3"));
  _icon_bar->registerCommand(ui::IconBar::View::Categories, command);
  _icon_bar->registerView(ui::IconBar::View::Categories,
                          std::make_shared<ui::CategoriesView>(*this));

  command = std::make_shared<ui::command::ToggleLog>(*_icon_bar);
  command_set->registerCommand(command, command::Shortcut("f4"));
  _icon_bar->registerCommand(ui::IconBar::View::Log, command);
  _icon_bar->registerView(ui::IconBar::View::Log, std::make_shared<ui::LogView>(*this));

  command = std::make_shared<ui::command::ToggleSettings>(*_icon_bar);
  command_set->registerCommand(command, command::Shortcut("ctrl+,"));
  _icon_bar->registerCommand(ui::IconBar::View::Settings, command);
  _icon_bar->registerView(ui::IconBar::View::Settings, std::make_shared<ui::SettingsView>(*this));

  command = std::make_shared<ui::command::CloseActiveView>();
  command_set->registerCommand(command, command::Shortcut("esc"));

  _panels.emplace_back(_icon_bar);
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


void UIViewer::updateWindowSize(const settings::Settings::Config &config,
                                const UICommandLineOptions *command_line_options)
{
  const auto at_exit = finally([this]() { _in_settings_notify = false; });
  _in_settings_notify = true;

  for (auto iter = config.extentions.begin(); iter != config.extentions.end(); ++iter)
  {
    if (iter->name() == kWindowSettingsName)
    {
      const auto &window_settings = *iter;
      // Default to settings.
      auto window_size = Magnum::Vector2i(
        static_cast<int>(
          window_settings[kWindowSettingsHorizontal].getProperty<WindowSizeProperty>()->value()),
        static_cast<int>(
          window_settings[kWindowSettingsVertical].getProperty<WindowSizeProperty>()->value()));

      // Override with command line options.
      if (command_line_options)
      {
        if (command_line_options->window.use_width)
        {
          window_size.x() = command_line_options->window.size.x();
        }
        if (command_line_options->window.use_height)
        {
          window_size.y() = command_line_options->window.size.y();
        }
      }

      if (!_in_viewport_event)
      {
        // We will end up here on startup and we can ensure the UI is created at the correct size
        // now. See comment in constructor.
        if (!_in_startup)
        {
          setWindowSize(window_size);
        }
        else
        {
          // See comment on _pending_window_size
          _pending_window_size = window_size;
          auto temp_size = Magnum::Vector2i{ 100, 100 };
          // Ensure the temp size is different from the desired size.
          if (temp_size.x() == window_size.x())
          {
            temp_size.x() += 1;
          }
          setWindowSize(temp_size);
        }
      }
      break;
    }
  }
}
}  // namespace tes::view
