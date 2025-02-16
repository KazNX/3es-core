#pragma once

#include <3rdEyeScene/ClientConfig.h>

#include <3esview/command/Command.h>

namespace tes::view::ui::command
{
class CloseActiveView : public tes::view::command::Command
{
public:
  CloseActiveView();

protected:
  bool checkAdmissible(Viewer &viewer) const override;
  view::command::CommandResult invoke(Viewer &viewer, const ExecInfo &info,
                                      const view::command::Args &args) override;
};
}  // namespace tes::view::ui::command
