#include "Stop.h"

#include <3esview/Viewer.h>
#include <3esview/data/NetworkThread.h>

namespace tes::view::command::playback
{
Stop::Stop()
  : Command("stop", Args())
{}


bool Stop::checkAdmissible(Viewer &viewer) const
{
  const auto data_thread = viewer.dataThread();
  const auto network_thread = std::dynamic_pointer_cast<data::NetworkThread>(data_thread);
  return network_thread && network_thread->isRecording() || !network_thread && data_thread;
}


CommandResult Stop::invoke(Viewer &viewer, const ExecInfo &info, const Args &args)
{
  (void)info;
  (void)args;
  const auto network_thread = std::dynamic_pointer_cast<data::NetworkThread>(viewer.dataThread());
  if (network_thread)
  {
    network_thread->endRecording();
  }
  else
  {
    viewer.closeOrDisconnect();
  }
  return { CommandResult::Code::Ok };
}
}  // namespace tes::view::command::playback
