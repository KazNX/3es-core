#ifndef TES_VIEW_COMMAND_RENDER_RESOLUTION_H
#define TES_VIEW_COMMAND_RENDER_RESOLUTION_H

#include <3esview/ViewConfig.h>

#include <3esview/command/Command.h>

namespace tes::view::command::resolution
{
/// Command to set the screen resolution.
///
/// @param width The new window width > 0 - required.
/// @param height The new window height > 0 - required.
class TES_VIEWER_API Resolution : public Command
{
public:
  Resolution();

protected:
  bool checkAdmissible(Viewer &viewer) const override;
  CommandResult invoke(Viewer &viewer, const ExecInfo &info, const Args &args) override;
};

/// Command to increase the window resolution to next larger preset.
class TES_VIEWER_API ResolutionIncrease : public Command
{
public:
  ResolutionIncrease();

protected:
  bool checkAdmissible(Viewer &viewer) const override;
  CommandResult invoke(Viewer &viewer, const ExecInfo &info, const Args &args) override;
};

/// Command to decrease the window resolution to next smaller preset.
class TES_VIEWER_API ResolutionDecrease : public Command
{
public:
  ResolutionDecrease();

protected:
  bool checkAdmissible(Viewer &viewer) const override;
  CommandResult invoke(Viewer &viewer, const ExecInfo &info, const Args &args) override;
};
}  // namespace tes::view::command::resolution

#endif  // TES_VIEW_COMMAND_RENDER_RESOLUTION_H
