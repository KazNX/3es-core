#ifndef TES_VIEW_VIEWER_H
#define TES_VIEW_VIEWER_H

#include "3esview/ViewConfig.h"

#include "camera/Fly.h"
#include "handler/Camera.h"
#include "ThirdEyeScene.h"

#include <filesystem>
#include <functional>
#include <optional>

namespace cxxopts
{
class Options;
class ParseResult;
}  // namespace cxxopts

namespace tes::view
{
namespace command
{
class Set;
class Shortcut;
}  // namespace command

namespace shaders
{
class Edl;
}  // namespace shaders

class EdlEffect;
class FboEffect;

namespace data
{
class DataThread;
}  // namespace data

class ViewerLog;

/// Structure into which we parse command line options.
///
/// This can be dervied for @c Viewer derivations to add their own command line options.
///
/// On constructing the @c Viewer, a @c CommandLineOptions object is created in the @c ViewArguments
/// according to the factory function passed to in - either an instance of this class or a
/// derivation thereof.
///
/// Once created, a @c CommandLineOptions object as the @c parse() function called. This calls the
/// @c addOptions() virtual function to add command line options. Derivations should override this
/// function, first calling the super class version, then adding their own command line options.
///
/// After parsing the @c validate() function is called to ensure the configuration is correct.
struct TES_VIEWER_API CommandLineOptions
{
  /// Indicates how to start the @c Viewer application.
  enum class StartupMode
  {
    /// An error has occured parsing the command line options. Best to show help and quit.
    Error,
    /// Normal UI startup mode.
    Normal,
    /// Show help and exit.
    Help,
    /// Start the UI and open a file.
    File,
    /// Start the UI and open a network connection.
    Host
  };

  struct ServerEndPoint
  {
    std::string host;
    uint16_t port = CommandLineOptions::defaultPort();
  };

  /// Get the default 3es server port.
  /// @return The default server port for 3es.
  [[nodiscard]] static uint16_t defaultPort();

  std::string filename;
  ServerEndPoint server;

  log::Level console_log_level = log::Level::Warn;

  CommandLineOptions() = default;
  CommandLineOptions(const CommandLineOptions &other) = default;
  CommandLineOptions(CommandLineOptions &&other) = default;
  virtual ~CommandLineOptions() = default;

  CommandLineOptions &operator=(const CommandLineOptions &other) = default;
  CommandLineOptions &operator=(CommandLineOptions &&other) = default;

  /// Called to parse the command line options.
  /// @param argc Command line option count - from @c main().
  /// @param argv Command line arguments - from @c main().
  /// @return The startup mode for the viewer.
  StartupMode parse(int argc, char **argv);

protected:
  virtual void addOptions(cxxopts::Options &parser);
  virtual bool validate(const cxxopts::ParseResult &parsed);
};

/// Command line arguments handler for @c Viewer.
///
/// This class is responsile for creating a @c CommandLineOptions object matching the command line
/// arguments.
///
/// The default implementation parses and returns a @c CommandLineOptions object. @c Viewer
/// derivations needing specialised @c CommandLineOptions should:
///
/// 1. Derive this class and implement a constructor @c MyViewArguments(int&,char**) which invokes
///   @c ViewArguments(int&,char**,OptionFactory)
/// 2. Pass an @c OptionFactory to the constructor above which creates the appropriate derivation of
///    @c CommandLineOptions
/// 3. Implement @c MyViewArguments::addOptions() and @c MyViewArguments::validate() .
/// 4. Implement the dervied @c Viewer constructor with the signature
///    `MyViewer(const MyViewArguments &)`.
struct TES_VIEWER_API ViewArguments : Magnum::Platform::Application::Arguments
{
  using Super = Magnum::Platform::Application::Arguments;
  using OptionFactory = std::function<std::unique_ptr<CommandLineOptions>()>;

  ViewArguments(int &argc, char **argv) noexcept
    : ViewArguments(argc, argv, []() { return std::make_unique<CommandLineOptions>(); })
  {}

  ViewArguments(int &argc, char **argv, OptionFactory option_factory) noexcept
    : Super(argc, argv)
    , _option_factory(std::move(option_factory))
  {}

  std::unique_ptr<CommandLineOptions> parseArgs(CommandLineOptions::StartupMode &startup_mode) const
  {
    auto opts = _option_factory();
    startup_mode = opts->parse(argc, argv);
    return opts;
  }

private:
  OptionFactory _option_factory;
};

class TES_VIEWER_API Viewer : public Magnum::Platform::Application
{
public:
  using Clock = std::chrono::steady_clock;

  explicit Viewer(const ViewArguments &arguments);
  Viewer(const ViewArguments &arguments, const std::vector<settings::Extension> &extended_settings);
  virtual ~Viewer();

  [[nodiscard]] std::shared_ptr<ThirdEyeScene> tes() const { return _tes; }

  [[nodiscard]] const std::shared_ptr<data::DataThread> &dataThread() const { return _data_thread; }
  [[nodiscard]] std::shared_ptr<command::Set> commands() { return _commands; }
  [[nodiscard]] const std::shared_ptr<command::Set> &commands() const { return _commands; }

  bool open(const std::filesystem::path &path);
  bool connect(const std::string &host, uint16_t port, bool allow_reconnect = true);
  bool closeOrDisconnect();

  void setContinuousSim(bool continuous);
  [[nodiscard]] bool continuousSim();

  void setActiveCamera(handler::Camera::CameraId id) { _active_recorded_camera = id; }
  void clearActiveCamera() { _active_recorded_camera.reset(); }
  [[nodiscard]] handler::Camera::CameraId activeCamera() const
  {
    return (isCameraActive()) ? *_active_recorded_camera : 0;
  }
  [[nodiscard]] bool isCameraActive() const { return _active_recorded_camera.has_value(); }

  [[nodiscard]] ViewerLog &logger() { return *_logger; }
  [[nodiscard]] const ViewerLog &logger() const { return *_logger; }

  [[nodiscard]] const CommandLineOptions &commandLineOptions() const
  {
    return *_command_line_options;
  }

  [[nodiscard]] std::shared_ptr<EdlEffect> edlEffect() const { return _edl_effect; }

  /// Hide base class @c setWindowSize() for DPI scaling issues.
  virtual void setWindowSize(const Magnum::Vector2i &size);

protected:
  /// Return value for @c onDrawStart()
  enum class DrawMode
  {
    /// Normal drawing.
    Normal,
    /// Modal drawing - disable normal input mode and key responses.
    /// Useful for when a UI has focus.
    Modal
  };

  /// Hook function called at the start of @c drawEvent(). Allows extensions such as UI.
  /// @param dt Time elapsed since the last draw (seconds).
  /// @return The @c DrawMode to proceed with.
  virtual DrawMode onDrawStart(float dt)
  {
    TES_UNUSED(dt);
    return DrawMode::Normal;
  }

  /// Hook function called at the start of @c drawEvent() before @c swapBuffers(). Allows extensions
  /// such as UI.
  /// @param dt Time elapsed since the last draw (seconds).
  virtual void onDrawComplete(float dt) { TES_UNUSED(dt); }

  void drawEvent() override;
  void viewportEvent(ViewportEvent &event) override;
  void keyPressEvent(KeyEvent &event) override;
  void keyReleaseEvent(KeyEvent &event) override;
  void mousePressEvent(MouseEvent &event) override;
  void mouseReleaseEvent(MouseEvent &event) override;
  void mouseMoveEvent(MouseMoveEvent &event) override;

  virtual void onReset();
  virtual void onCameraSettingsChange(const settings::Settings::Config &config);
  virtual void onLogSettingsChange(const settings::Settings::Config &config);
  virtual void onRenderSettingsChange(const settings::Settings::Config &config);
  virtual void onPlaybackSettingsChange(const settings::Settings::Config &config);

private:
  enum class EdlParam
  {
    LinearScale,
    ExponentialScale,
    Radius
  };

  void updateCamera(float dt, bool allow_user_input);
  void updateCameraInput(float dt, camera::Camera &camera);

  /// Check if we are waiting on the data thread to catch up to a specific frame.
  bool waitingForCatchup();

  void checkShortcuts(KeyEvent &event);
  /// Give @p shortcut a score based on the number of keys matched against @p event.
  ///
  /// Score 1 for each key and each modifier matched.
  static int scoreShortcut(const command::Shortcut &shortcut, const KeyEvent &event);

  bool handleStartupArgs(CommandLineOptions::StartupMode startup_mode);

  /// Update keyframes to the @c _data_thread if it's a @c StreamThread .
  /// @param config New playback settings. Only keyframe settings are used.
  void updateStreamThreadKeyframesConfig(const settings::Playback &config);

  struct KeyAxis
  {
    KeyEvent::Key key;
    unsigned axis = 0;
    bool negate = false;
    bool active = false;
  };

  std::shared_ptr<EdlEffect> _edl_effect;

  std::shared_ptr<ThirdEyeScene> _tes;
  std::shared_ptr<data::DataThread> _data_thread;
  std::shared_ptr<command::Set> _commands;
  std::unique_ptr<ViewerLog> _logger;

  Clock::time_point _last_sim_time = Clock::now();

  camera::Camera _camera = {};
  /// ID of the recorded camera to use the transform of.
  /// This identifies one of the hander::Camera cameras.
  std::optional<handler::Camera::CameraId> _active_recorded_camera;
  camera::Fly _fly;

  bool _mouse_rotation_active = false;
  bool _continuous_sim = true;

  std::vector<KeyAxis> _move_keys;
  std::vector<KeyAxis> _rotate_keys;

  std::unique_ptr<CommandLineOptions> _command_line_options;
};
}  // namespace tes::view

#endif  // TES_VIEW_VIEWER_H
