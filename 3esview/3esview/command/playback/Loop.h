#pragma once

#include <3esview/ViewConfig.h>

#include <3esview/command/Command.h>

namespace tes::view::command::playback
{
class TES_VIEWER_API Loop : public Command
{
public:
  Loop();

protected:
  bool checkAdmissible(Viewer &viewer) const override;
  CommandResult invoke(Viewer &viewer, const ExecInfo &info, const Args &args) override;
};
}  // namespace tes::view::command::playback
