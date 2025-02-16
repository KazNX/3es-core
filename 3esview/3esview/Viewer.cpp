#include "Viewer.h"

#include "EdlEffect.h"
#include "ViewerLog.h"

#include "command/DefaultCommands.h"
#include "command/Set.h"

#include "data/NetworkThread.h"
#include "data/StreamThread.h"

#include "painter/Arrow.h"
#include "painter/Box.h"
#include "painter/Capsule.h"
#include "painter/Cylinder.h"
#include "painter/Plane.h"
#include "painter/Pose.h"
#include "painter/Sphere.h"
#include "painter/Star.h"

#include <3escore/Log.h>
#include <3escore/Maths.h>
#include <3escore/Server.h>

#include <Magnum/GL/Context.h>
#include <Magnum/GL/DefaultFramebuffer.h>
#include <Magnum/GL/RenderbufferFormat.h>
#include <Magnum/GL/Renderer.h>
#include <Magnum/GL/TextureFormat.h>
#include <Magnum/GL/Version.h>

#include <fstream>
#include <iostream>

#include <cxxopts.hpp>

// Things to learn about:
// - UI

// Things to implement:
// - mesh renderer

namespace tes::view
{
namespace
{
void focusCallback(GLFWwindow *window, int focused)
{
  auto *app = static_cast<Magnum::Platform::GlfwApplication *>(glfwGetWindowUserPointer(window));
  auto *viewer = dynamic_cast<Viewer *>(app);
  if (viewer)
  {
    viewer->setContinuousSim(focused != 0);
  }
}
}  // namespace


uint16_t CommandLineOptions::defaultPort()
{
  return ServerSettings().listen_port;
}

CommandLineOptions::StartupMode CommandLineOptions::parse(int argc, char **argv)
{
  const auto program_name = std::filesystem::path(argv[0]).filename().string();
  cxxopts::Options opt_parse(program_name, "3rd Eye Scene viewer.");

  addOptions(opt_parse);

  try
  {
    cxxopts::ParseResult parsed = opt_parse.parse(argc, argv);

    if (parsed.count("help"))
    {
      std::cout << opt_parse.help() << std::endl;
      // Help already shown.
      return StartupMode::Help;
    }

    if (!validate(parsed))
    {
      std::cerr << "Argument validation error." << std::endl;
      return StartupMode::Error;
    }
  }
  catch (const cxxopts::exceptions::parsing &e)
  {
    std::cerr << "Argument error\n" << e.what() << std::endl;
    return StartupMode::Error;
  }

  if (!filename.empty())
  {
    return StartupMode::File;
  }

  if (!server.host.empty())
  {
    return StartupMode::Host;
  }

  return StartupMode::Normal;
}


bool CommandLineOptions::validate([[maybe_unused]] const cxxopts::ParseResult &parsed)
{
  // Noting to validate in the base implementation.
  return true;
}


void CommandLineOptions::addOptions(cxxopts::Options &parser)
{
  // clang-format off
  parser.add_options()
    ("help", "Show command line help.")
    ("file", "Start the UI and open this file for playback. Takes precedence over --host.", cxxopts::value(filename))
    ("host", "Start the UI and open a connection to this host URL/IP. Use --port to select the port number.", cxxopts::value(server.host))
    ("port", "The port number to use with --host", cxxopts::value(server.port)->default_value(std::to_string(server.port)))
    ("log-level", "Minimum logging level to display: [trace, info, warn, error].", cxxopts::value(console_log_level))
    ;
  // clang-format on
}


Viewer::Viewer(const ViewArguments &arguments)
  : Viewer(arguments, {})
{}


Viewer::Viewer(const ViewArguments &arguments,
               const std::vector<settings::Extension> &extended_settings)
  : Magnum::Platform::Application{ arguments,
                                   Configuration{}
                                     .setTitle("3rd Eye Scene Viewer")
                                     .setWindowFlags(Configuration::WindowFlag::Resizable) }
  , _tes(std::make_shared<ThirdEyeScene>(extended_settings))
  , _commands(std::make_shared<command::Set>())
  , _logger(std::make_unique<ViewerLog>())
  , _move_keys({
      { KeyEvent::Key::A, 0, true },         //
      { KeyEvent::Key::Left, 0, true },      //
      { KeyEvent::Key::D, 0, false },        //
      { KeyEvent::Key::Right, 0, false },    //
      { KeyEvent::Key::W, 1, false },        //
      { KeyEvent::Key::Up, 1, false },       //
      { KeyEvent::Key::S, 1, true },         //
      { KeyEvent::Key::Down, 1, true },      //
      { KeyEvent::Key::R, 2, false },        //
      { KeyEvent::Key::PageUp, 2, false },   //
      { KeyEvent::Key::F, 2, true },         //
      { KeyEvent::Key::PageDown, 2, true },  //
    })
  , _rotate_keys({
      { KeyEvent::Key::T, 0, false },  //
      { KeyEvent::Key::G, 0, true },   //
      { KeyEvent::Key::Q, 1, false },  //
      { KeyEvent::Key::E, 1, true },   //
    })
{
  _edl_effect = std::make_shared<EdlEffect>(Magnum::GL::defaultFramebuffer.viewport());
  command::registerDefaultCommands(*_commands);

  CommandLineOptions::StartupMode startup_mode;
  _command_line_options = arguments.parseArgs(startup_mode);
  if (!handleStartupArgs(startup_mode))
  {
    exit();
  }

  // GLFW specific.
  // Bind a callback to force continuous sim while focused.
  // glfwSetWindowUserPointer(window(), this);
  glfwSetWindowFocusCallback(window(), focusCallback);

  const auto config = _tes->settings().config();
  _camera.position = { 0, -5, 0 };
  onCameraSettingsChange(config);
  onLogSettingsChange(config);
  onRenderSettingsChange(config);
  onPlaybackSettingsChange(config);

  _tes->settings().addObserver(
    settings::Settings::Category::Camera,
    [this](const settings::Settings::Config &config) { onCameraSettingsChange(config); });
  _tes->settings().addObserver(
    settings::Settings::Category::Log,
    [this](const settings::Settings::Config &config) { onLogSettingsChange(config); });
  _tes->settings().addObserver(
    settings::Settings::Category::Render,
    [this](const settings::Settings::Config &config) { onRenderSettingsChange(config); });
  _tes->settings().addObserver(
    settings::Settings::Category::Playback,
    [this](const settings::Settings::Config &config) { onPlaybackSettingsChange(config); });
  // Install the logger function.
  log::setLogger(
    [this](log::Level level, const std::string &message) { _logger->log(level, message); });
  _logger->setConsoleLogLevel(_command_line_options->console_log_level);
}


Viewer::~Viewer()
{
  // Uninstall the logger.
  setLogger(log::defaultLogger);
  closeOrDisconnect();
}


bool Viewer::open(const std::filesystem::path &path)
{
  closeOrDisconnect();
  _tes->reset();
  std::ifstream file(path.string(), std::ios::binary);
  if (!file.is_open())
  {
    return false;
  }

  const auto config = _tes->settings().config();
  _data_thread =
    std::make_shared<data::StreamThread>(_tes, std::make_shared<std::ifstream>(std::move(file)));
  _data_thread->setLooping(config.playback.looping.value());
  updateStreamThreadKeyframesConfig(config.playback);
  return true;
}


bool Viewer::connect(const std::string &host, uint16_t port, bool allow_reconnect)
{
  closeOrDisconnect();
  _tes->reset();
  auto net_thread = std::make_shared<data::NetworkThread>(_tes, host, port, allow_reconnect);
  _data_thread = net_thread;
  if (!allow_reconnect)
  {
    // Reconnection not allowed. Wait until the network thread has tried to connect...
    const auto start_time = std::chrono::steady_clock::now();
    // ...but don't wait forever.
    const auto timeout = std::chrono::seconds(5);
    while (!net_thread->connectionAttempted() &&
           (std::chrono::steady_clock::now() - start_time) < timeout)
    {
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    return net_thread->connected();
  }
  return true;
}


bool Viewer::closeOrDisconnect()
{
  _active_recorded_camera.reset();
  if (_data_thread)
  {
    _data_thread->stop();
    _data_thread->join();
    _data_thread = nullptr;
    return true;
  }
  else
  {
    // Reset existing data on second close/reset request.
    _tes->reset();
  }
  return false;
}


void Viewer::setContinuousSim(bool continuous)
{
  if (_continuous_sim != continuous)
  {
    _continuous_sim = continuous;
    if (continuous)
    {
      _last_sim_time = Clock::now();
    }
  }
}

bool Viewer::continuousSim()
{
  // Check forcing continuous mode.
  if (_continuous_sim || _mouse_rotation_active || _data_thread)
  {
    return true;
  }

  // Check keys.
  bool continuous_sim = false;
  for (const auto &key : _move_keys)
  {
    continuous_sim = key.active || continuous_sim;
  }
  for (const auto &key : _rotate_keys)
  {
    continuous_sim = key.active || continuous_sim;
  }
  return continuous_sim;
}


void Viewer::setWindowSize(const Magnum::Vector2i &size)
{
  // Copy the window size as we can end up modifying it again.
  // Hack: adjust for dpiScaling() const windowSize() doesn't consider it, but setWindowSize()
  // does.
  const auto dpi_scaling = dpiScaling();
  const auto new_size = (Magnum::Vector2(size) / dpi_scaling) + Magnum::Vector2(0.5f, 0.5f);
  Magnum::Platform::Application::setWindowSize(Magnum::Vector2i(new_size));
}


void Viewer::drawEvent()
{
  using namespace Magnum::Math::Literals;

  const bool waiting_for_catchup = waitingForCatchup();

  const auto now = Clock::now();
  const auto delta_time = now - _last_sim_time;
  _last_sim_time = now;
  const float dt = std::chrono::duration_cast<std::chrono::duration<float>>(delta_time).count();

  const auto draw_mode = onDrawStart(dt);

  updateCamera(dt, draw_mode == DrawMode::Normal);

  _tes->render(dt, windowSize(), !waiting_for_catchup);

  onDrawComplete(dt);

  if (!waiting_for_catchup)
  {
  swapBuffers();
  }

  if (waiting_for_catchup || continuousSim() || isTextInputActive())
  {
    redraw();
  }
}


void Viewer::viewportEvent(ViewportEvent &event)
{
  const auto viewport = Magnum::Range2Di{ {}, event.framebufferSize() };
  Magnum::GL::defaultFramebuffer.setViewport(viewport);
  _edl_effect->viewportChange(viewport);
}


void Viewer::keyPressEvent(KeyEvent &event)
{
  // Start with the shortcuts.
  checkShortcuts(event);
  if (event.isAccepted())
  {
    return;
  }

  bool dirty = false;
  for (auto &key : _move_keys)
  {
    if (event.key() == key.key)
    {
      key.active = true;
      event.setAccepted();
      dirty = true;
    }
  }

  for (auto &key : _rotate_keys)
  {
    if (event.key() == key.key)
    {
      key.active = true;
      event.setAccepted();
      dirty = true;
    }
  }

  if (event.key() == KeyEvent::Key::LeftShift)
  {
    const float fast_multiplier = 2.0f;
    _fly.setMoveMultiplier(fast_multiplier);
    // _fly.setRotationMultiplier(fast_multiplier);
    event.setAccepted();
  }

  if (dirty)
  {
    redraw();
  }
}


void Viewer::keyReleaseEvent(KeyEvent &event)
{
  bool dirty = false;
  for (auto &key : _move_keys)
  {
    if (event.key() == key.key)
    {
      key.active = false;
      event.setAccepted();
      dirty = true;
    }
  }

  for (auto &key : _rotate_keys)
  {
    if (event.key() == key.key)
    {
      key.active = false;
      event.setAccepted();
      dirty = true;
    }
  }

  if (event.key() == KeyEvent::Key::LeftShift)
  {
    _fly.setMoveMultiplier(1.0f);
    _fly.setRotationMultiplier(1.0f);
    event.setAccepted();
  }

  if (dirty)
  {
    redraw();
  }
}


void Viewer::mousePressEvent(MouseEvent &event)
{
  if (event.button() != MouseEvent::Button::Left)
    return;

  _mouse_rotation_active = true;
  event.setAccepted();
}


void Viewer::mouseReleaseEvent(MouseEvent &event)
{
  _mouse_rotation_active = false;

  event.setAccepted();
  redraw();
}


void Viewer::mouseMoveEvent(MouseMoveEvent &event)
{
  using namespace Magnum::Math::Literals;
  if (!(event.buttons() & MouseMoveEvent::Button::Left))
  {
    return;
  }

  _fly.updateMouse(float(event.relativePosition().x()), float(event.relativePosition().y()),
                   _camera);

  event.setAccepted();
  redraw();
}


void Viewer::onReset()
{
  _active_recorded_camera.reset();
}


void Viewer::onCameraSettingsChange(const settings::Settings::Config &config)
{
  _camera.clip_far = config.camera.far_clip.value();
  _camera.clip_near = config.camera.near_clip.value();
  _camera.fov_horizontal_deg = config.camera.fov.value();
}


void Viewer::onLogSettingsChange(const settings::Settings::Config &config)
{
  _logger->setMaxLines(config.log.log_history.value());
}


void Viewer::onRenderSettingsChange(const settings::Settings::Config &config)
{
  const auto &render = config.render;
  _edl_effect->setLinearScale(render.edl_linear_scale.value());
  _edl_effect->setExponentialScale(render.edl_exponential_scale.value());
  _edl_effect->setRadius(static_cast<float>(render.edl_radius.value()));
  if (render.use_edl_shader.value())
  {
    _tes->setActiveFboEffect(_edl_effect);
  }
  else
  {
    _tes->clearActiveFboEffect();
  }
}


void Viewer::onPlaybackSettingsChange(const settings::Settings::Config &config)
{
  if (_data_thread)
  {
    _data_thread->setLooping(config.playback.looping.value());
    updateStreamThreadKeyframesConfig(config.playback);
  }
}


void Viewer::updateCamera(float dt, bool allow_user_input)
{
  if (_active_recorded_camera.has_value())
  {
    auto camera_handler =
      std::dynamic_pointer_cast<handler::Camera>(_tes->messageHandler(MtCamera));
    if (camera_handler)
    {
      const auto config = _tes->settings().config().camera;
      // Check camera handler for an active camera.
      camera::Camera remote_camera = {};
      camera_handler->lookup(*_active_recorded_camera, remote_camera);
      if (!config.allow_remote_settings.value())
      {
        // Don't allow remote camera settings. Keep the user settings.
        remote_camera.clip_far = _camera.clip_far;
        remote_camera.clip_near = _camera.clip_near;
        remote_camera.fov_horizontal_deg = _camera.fov_horizontal_deg;
      }
      _tes->setCamera(remote_camera);
    }
    allow_user_input = false;
  }

  if (allow_user_input)
  {
    updateCameraInput(dt, _camera);
    _tes->setCamera(_camera);
  }
  else
  {
    _mouse_rotation_active = false;
  }
}

void Viewer::updateCameraInput(float dt, camera::Camera &camera)
{
  Magnum::Vector3i key_translation(0);
  Magnum::Vector3i key_rotation(0);
  for (const auto &key : _move_keys)
  {
    if (key.active)
    {
      key_translation[key.axis] += (!key.negate) ? 1 : -1;
    }
  }
  for (const auto &key : _rotate_keys)
  {
    if (key.active)
    {
      key_rotation[key.axis] += (!key.negate) ? 1 : -1;
    }
  }

  _fly.updateKeys(dt, key_translation, key_rotation, camera);
}


bool Viewer::waitingForCatchup()
{
  std::optional<tes::view::FrameNumber> target_frame =
    (_data_thread && _data_thread->paused()) ? _data_thread->targetFrame() : std::nullopt;

  return target_frame.has_value();
}


void Viewer::checkShortcuts(KeyEvent &event)
{
  if (!_commands || event.isRepeated())
  {
    return;
  }

  command::Set::Item best_shortcut = {};
  int best_score = 0;
  for (const auto &[name, shortcut] : _commands->commands())
  {
    const int score = scoreShortcut(shortcut.shortcut, event);
    if (score > best_score && shortcut.command->admissible(*this))
    {
      best_shortcut = shortcut;
      best_score = score;
    }
  }

  if (best_shortcut.command)
  {
    log::info("Invoke shortcut command '", best_shortcut.command->name(), "'");
    event.setAccepted();
    const auto result = best_shortcut.command->invoke(*this);
    switch (result.code())
    {
    case command::CommandResult::Code::Ok:
      log::info("Invoked shortcut command '", best_shortcut.command->name(), "'");
      break;
    case command::CommandResult::Code::Cancel:
      log::info("Cancelled shortcut command '", best_shortcut.command->name(), "'");
      break;
    default:
      log::error("Failed shortcut command '", best_shortcut.command->name(),
                 "' : ", result.reason());
      break;
    }
  }
}


int Viewer::scoreShortcut(const command::Shortcut &shortcut, const KeyEvent &event)
{
  if (event.key() != shortcut.key())
  {
    return 0;
  }

  const auto modifiers =
    static_cast<unsigned>(static_cast<int>(event.modifiers())) & shortcut.modifierFlags();
  if (modifiers != shortcut.modifierFlags())
  {
    return 0;
  }

  int score = 1;                  // for key match.
  score += countBits(modifiers);  // for each modifier matched
  return score;
}


bool Viewer::handleStartupArgs(CommandLineOptions::StartupMode startup_mode)
{
  using StartupMode = CommandLineOptions::StartupMode;
  switch (startup_mode)
  {
  case StartupMode::Error:
  case StartupMode::Help:
    // Do not start UI.
    return false;
  case StartupMode::Normal:
    break;
  case StartupMode::File:
    open(_command_line_options->filename);
    break;
  case StartupMode::Host: {
    // Extract a port number if possible.
    connect(_command_line_options->server.host, _command_line_options->server.port, true);
    break;
  default:
    break;
  }
  }

  return true;
}


void Viewer::updateStreamThreadKeyframesConfig(const settings::Playback &config)
{
  if (auto stream_thread = std::dynamic_pointer_cast<data::StreamThread>(_data_thread))
  {
    stream_thread->setAllowKeyframes(config.allow_key_frames.value());
    stream_thread->setKeyframeInterval(config.keyframe_every_frames.value());
    stream_thread->setKeyframeSizeInterval(config.keyframe_every_mib.value());
  }
}
}  // namespace tes::view
