#include "Edl.h"

#include <3esview/EdlEffect.h>
#include <3esview/Viewer.h>

namespace tes::view::command::render
{
Edl::Edl()
  : Command("edl", Args(true))
{}


bool Edl::checkAdmissible([[maybe_unused]] Viewer &viewer) const
{
  return viewer.edlEffect() != nullptr && viewer.tes() != nullptr;
}


CommandResult Edl::invoke(Viewer &viewer, [[maybe_unused]] const ExecInfo &info,
                          [[maybe_unused]] const Args &args)
{
  bool turn_on = false;
  const auto scene = viewer.tes();
  if (!args.empty())
  {
    turn_on = arg<bool>(0, args);
  }
  else
  {
    turn_on = scene->activeFboEffect() != viewer.edlEffect();
  }

  if (turn_on)
  {
    scene->setActiveFboEffect(viewer.edlEffect());
  }
  else
  {
    scene->clearActiveFboEffect();
  }
  return { CommandResult::Code::Ok };
}
}  // namespace tes::view::command::render
