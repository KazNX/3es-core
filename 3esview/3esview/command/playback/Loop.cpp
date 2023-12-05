#include "Loop.h"

#include <3esview/Viewer.h>
#include <3esview/data/DataThread.h>

namespace tes::view::command::playback
{
Loop::Loop()
  : Command("loop", Args(true))
{}


bool Loop::checkAdmissible([[maybe_unused]] Viewer &viewer) const
{
  // We can always change the config value without a playing stream.
  return true;
}


CommandResult Loop::invoke(Viewer &viewer, const ExecInfo &info, const Args &args)
{
  (void)info;
  const auto loop = arg<bool>(0, args);

  // Update config
  auto playback_settings = viewer.tes()->settings().config().playback;
  playback_settings.looping.setValue(loop);
  viewer.tes()->settings().update(playback_settings);

  auto stream = viewer.dataThread();
  if (stream)
  {
    stream->setLooping(loop);
  }
  return { CommandResult::Code::Ok };
}
}  // namespace tes::view::command::playback
