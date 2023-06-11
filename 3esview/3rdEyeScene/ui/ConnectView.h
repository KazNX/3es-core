//
// Author: Kazys Stepanas
//
#ifndef TRD_EYE_SCENE_UI_CONNECT_VIEW_H
#define TRD_EYE_SCENE_UI_CONNECT_VIEW_H

#include <3rdEyeScene/ClientConfig.h>

#include "Panel.h"

#include <3escore/Server.h>

#include <Magnum/GL/Texture.h>

#include <array>
#include <memory>

namespace tes::view
{
class DataThread;
}

namespace tes::view::command
{
class Command;
}

namespace tes::view::ui
{
class ConnectView : public Panel
{
public:
  /// An enumeration of the actions which can be triggered by the playback bar.
  ///
  /// @c Command objects are to be registered with each action to effect those actions.
  enum Action : unsigned
  {
    /// Connect with selected IP and port.
    Connect,
    /// Disconnect from the current session.
    Disconnect,

    /// Number of @c Actions - used for array sizes.
    Count
  };

  ConnectView(Viewer &viewer);

  void registerAction(Action action, std::shared_ptr<command::Command> command)
  {
    if (action != Action::Count)
    {
      _actions[static_cast<int>(action)] = command;
    }
  }

  std::shared_ptr<command::Command> command(Action action) const
  {
    if (action != Action::Count)
    {
      return _actions[static_cast<unsigned>(action)];
    }
    return {};
  }

private:
  void drawContent(Magnum::ImGuiIntegration::Context &ui, Window &window) override;
  void updateHistory(const std::string &host, uint16_t port);

  using ActionSet =
    std::array<std::shared_ptr<command::Command>, static_cast<unsigned>(Action::Count)>;

  std::string _host = "127.0.0.1";
  int _port = ServerSettings::kDefaultPort;
  std::array<char, 4 * 1024> _host_buffer = {};
  ActionSet _actions;
};
}  // namespace tes::view::ui

#endif  // TRD_EYE_SCENE_UI_CONNECT_VIEW_H
