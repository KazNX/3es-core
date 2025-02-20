#pragma once

#include <3esview/ViewConfig.h>

#include <3esview/command/Command.h>

namespace tes::view::command::render
{
/// Command to turn on/off EDL
///
/// @param on New EDL state - optional. Toggle when omitted.
class TES_VIEWER_API Edl : public Command
{
public:
  Edl();

protected:
  bool checkAdmissible(Viewer &viewer) const override;
  CommandResult invoke(Viewer &viewer, const ExecInfo &info, const Args &args) override;
};
}  // namespace tes::view::command::render
