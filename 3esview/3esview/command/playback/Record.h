//
// Author: Kazys Stepanas
//
#ifndef TES_VIEW_COMMAND_PLAYBACK_RECORD_H
#define TES_VIEW_COMMAND_PLAYBACK_RECORD_H

#include <3esview/ViewConfig.h>

#include <3esview/command/Command.h>

namespace tes::view::command::playback
{
/// Command to start recording to file.
///
/// Only valid for a network stream.
///
/// Shows a file save dialog if no file argument is given.
class TES_VIEWER_API Record : public Command
{
public:
  Record();

protected:
  bool checkAdmissible(Viewer &viewer) const override;
  CommandResult invoke(Viewer &viewer, const ExecInfo &info, const Args &args) override;

  /// Select a file from a dialog.
  std::string fromDialog();
};
}  // namespace tes::view::command::playback

#endif  // TES_VIEW_COMMAND_PLAYBACK_RECORD_H
