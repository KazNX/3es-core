#include "OpenFile.h"

#include <3esview/Viewer.h>

#include <3esview/util/CStrPtr.h>

#include <tinyfiledialogs.h>

namespace tes::view::command::connection
{
OpenFile::OpenFile()
  : Command("openFile", Args(std::string()))
{}


bool OpenFile::checkAdmissible(Viewer &viewer) const
{
  return viewer.dataThread() == nullptr;
}


CommandResult OpenFile::invoke(Viewer &viewer, const ExecInfo &info, const Args &args)
{
  (void)info;
  std::string filename;
  if (!args.empty())
  {
    filename = args.at<std::string>(0);
  }
  else
  {
    filename = fromDialog();
  }

  if (filename.empty())
  {
    return { CommandResult::Code::Cancel };
  }

  if (!viewer.open(filename))
  {
    return { CommandResult::Code::Failed, "Failed to open " + filename };
  }
  return { CommandResult::Code::Ok };
}


std::string OpenFile::fromDialog()
{
  // // const char *filter_list = "3rd Eye Scene files (*.3es),*.3es";
  std::array<const char *, 2> filters = { "*.3es" };
  util::CStrPtr selection{ tinyfd_openFileDialog("Open file", nullptr, 1, filters.data(), nullptr,
                                                 0) };
  if (selection)
  {
    return { selection.get() };
  }

  return std::string();
}
}  // namespace tes::view::command::connection
