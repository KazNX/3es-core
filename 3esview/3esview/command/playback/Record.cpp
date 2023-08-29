//
// Author: Kazys Stepanas
//
#include "Record.h"

#include <3esview/data/NetworkThread.h>
#include <3esview/Viewer.h>

#include <3esview/util/CStrPtr.h>

#include <tinyfiledialogs.h>

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
  // // const char *filter_list = "3rd Eye Scene files (*.3es),*.3es";
  std::array<const char *, 2> filters = { "*.3es" };
  util::CStrPtr selection{ tinyfd_saveFileDialog("Save file", nullptr, 1, filters.data(),
                                                 nullptr) };

  if (selection)
  {
    return { selection.get() };
  }

  return std::string();
}
}  // namespace tes::view::command::playback
