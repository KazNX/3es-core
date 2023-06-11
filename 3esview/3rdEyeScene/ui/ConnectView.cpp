//
// Author: Kazys Stepanas
//
#include "ConnectView.h"

#include <3esview/command/Set.h>
#include <3esview/Viewer.h>

#include <3escore/CoreUtil.h>
#include <3escore/Log.h>

#include <Magnum/ImGuiIntegration/Context.hpp>

#include <iterator>

namespace tes::view::ui
{
ConnectView::ConnectView(Viewer &viewer)
  : Panel("Connect", viewer)
{
  registerAction(Action::Connect, viewer.commands()->lookupName("openTcp").command);
  registerAction(Action::Disconnect, viewer.commands()->lookupName("close").command);
}


void ConnectView::drawContent(Magnum::ImGuiIntegration::Context &ui, Window &window)
{
  TES_UNUSED(ui);
  TES_UNUSED(window);

  std::copy(_host.begin(), _host.end(), _host_buffer.begin());
  _host_buffer[_host.length()] = '\0';
  if (ImGui::InputText("Host", _host_buffer.data(), _host_buffer.size()))
  {
    _host = _host_buffer.data();
  }
  if (ImGui::InputInt("Port", &_port))
  {
    const int max_port = 0xffff;
    _port = std::max(0, std::min(_port, max_port));
  }

  std::string connect_ip = {};
  uint16_t connect_port = 0;

  const auto connect_cmd = command(Action::Connect);
  const bool can_connect = connect_cmd && connect_cmd->admissible(viewer());
  ImGui::BeginDisabled(!can_connect);
  if (ImGui::Button("Connect"))
  {
    // Send connect command.
    connect_ip = _host;
    connect_port = int_cast<uint16_t>(_port);
  }
  ImGui::EndDisabled();

  ImGui::SameLine();

  const auto disconnect_cmd = command(Action::Disconnect);
  const bool can_disconnect = disconnect_cmd && disconnect_cmd->admissible(viewer());
  ImGui::BeginDisabled(!can_disconnect);
  if (ImGui::Button("Disconnect"))
  {
    // Send disconnect command.
    if (can_disconnect)
    {
      disconnect_cmd->invoke(viewer());
    }
  }
  ImGui::EndDisabled();

  ImGui::Separator();

  // Show history
  ImGui::BeginDisabled(!can_connect);
  const auto connection = viewer().tes()->settings().config().connection;
  for (const auto &[history_host, history_port] : connection.history)
  {
    const std::string label = history_host + ":" + std::to_string(history_port);
    if (ImGui::Button(label.c_str()))
    {
      // Send connect command.
      connect_ip = history_host;
      connect_port = int_cast<uint16_t>(history_port);
    }
  }
  ImGui::EndDisabled();

  if (!connect_ip.empty() && can_connect)
  {
    if (connect_cmd->invoke(viewer(), { connect_ip, connect_port }).code() ==
        command::CommandResult::Code::Ok)
    {
      updateHistory(connect_ip, connect_port);
    }
  }
}


void ConnectView::updateHistory(const std::string &host, uint16_t port)
{
  // Add to front of the history list.
  auto connection = viewer().tes()->settings().config().connection;
  connection.history.insert(connection.history.begin(), { host, port });
  // Remove if existing.
  auto iter = connection.history.begin();
  for (++iter; iter != connection.history.end(); ++iter)
  {
    if (*iter == connection.history[0])
    {
      connection.history.erase(iter);
      break;
    }
  }

  // Resize history if too big.
  if (connection.history.size() > settings::Connection::kHistoryLimit)
  {
    connection.history.resize(settings::Connection::kHistoryLimit);
  }

  // And write.
  viewer().tes()->settings().update(connection);
}
}  // namespace tes::view::ui
