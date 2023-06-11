//
// Author: Kazys Stepanas
//
#include "Record.h"

#include <3esview/data/NetworkThread.h>
#include <3esview/Viewer.h>

#include <nfd.h>

namespace tes::view::command::playback
{
Record::Record()
  : Command("record", Args(std::string()))
{}


bool Record::checkAdmissible(Viewer &viewer) const
{
  const auto network_thread = std::dynamic_pointer_cast<data::NetworkThread>(viewer.dataThread());
  return network_thread != nullptr && !network_thread->isRecording();
}


CommandResult Record::invoke(Viewer &viewer, const ExecInfo &info, const Args &args)
{
  (void)info;
  const auto network_thread = std::dynamic_pointer_cast<data::NetworkThread>(viewer.dataThread());
  if (!network_thread)
  {
    return { CommandResult::Code::Inadmissible, "No network thread active." };
  }

  if (network_thread->isRecording())
  {
    return { CommandResult::Code::Inadmissible, "Already recording." };
  }

  std::string filename;

  if (!args.empty())
  {
    filename = arg<std::string>(0, args);
  }
  else
  {
    filename = fromDialog();
  }

  if (filename.empty())
  {
    return { CommandResult::Code::InvalidArguments, "Recoding filename required." };
  }

  network_thread->startRecording(filename);
  return { CommandResult::Code::Ok };
}


std::string Record::fromDialog()
{
  // const nfdchar_t *filter_list = "3rd Eye Scene files (*.3es),*.3es";
  // Seems the vcpkg version of nativefiledialog only supports file extension strings.
  const nfdchar_t *filter_list = "3es";
  // TODO(KS): cache the last use path and reload it.
  const nfdchar_t *default_path = "";
  nfdchar_t *selected_path = nullptr;
  nfdresult_t result = NFD_SaveDialog(filter_list, default_path, &selected_path);

  const std::string path = (selected_path) ? selected_path : std::string();
  free(selected_path);

  if (result == NFD_OKAY)
  {
    return path;
  }
  return std::string();
}
}  // namespace tes::view::command::playback
