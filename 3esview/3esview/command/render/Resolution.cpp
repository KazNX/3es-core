#include "Resolution.h"

#include <3esview/Viewer.h>

namespace tes::view::command::render
{
namespace
{
constexpr std::array<Magnum::Vector2i, 16> kPresetResolutions =  //
  {
    Magnum::Vector2i{ 640, 480 },    //
    Magnum::Vector2i{ 800, 600 },    //
    Magnum::Vector2i{ 1024, 768 },   //
    Magnum::Vector2i{ 1280, 1024 },  //
    Magnum::Vector2i{ 1360, 768 },   //
    Magnum::Vector2i{ 1440, 900 },   //
    Magnum::Vector2i{ 1600, 900 },   //
    Magnum::Vector2i{ 1600, 1200 },  //
    Magnum::Vector2i{ 1920, 1080 },  //
    Magnum::Vector2i{ 1920, 1200 },  //
    Magnum::Vector2i{ 2048, 1152 },  //
    Magnum::Vector2i{ 2048, 1536 },  //
    Magnum::Vector2i{ 2560, 1440 },  //
    Magnum::Vector2i{ 2560, 1600 },  //
    Magnum::Vector2i{ 3440, 1440 },  //
    Magnum::Vector2i{ 3440, 2160 },  //
  };

bool resolutionLess(const Magnum::Vector2i &a, const Magnum::Vector2i &b)
{
  return a.x() < b.x() || a.x() == b.x() && a.y() < b.y();
}

Magnum::Vector2i nextResolutionDown(Magnum::Vector2i resolution)
{
  auto last = resolution;
  for (const auto preset : kPresetResolutions)
  {
    volatile auto cmp = resolution < preset;
    if (resolutionLess(resolution, preset) || resolution == preset)
    {
      return last;
    }
    last = preset;
  }
  return resolution;
}

Magnum::Vector2i nextResolutionUp(Magnum::Vector2i resolution)
{
  for (const auto preset : kPresetResolutions)
  {
    if (resolutionLess(resolution, preset))
    {
      return preset;
    }
  }
  return resolution;
}
}  // namespace

Resolution::Resolution()
  : Command("resolution", Args(0, 0))
{}


bool Resolution::checkAdmissible([[maybe_unused]] Viewer &viewer) const
{
  return true;
}


CommandResult Resolution::invoke(Viewer &viewer, [[maybe_unused]] const ExecInfo &info,
                                 [[maybe_unused]] const Args &args)
{
  const auto width = arg<int>(0, args);
  const auto height = arg<int>(1, args);
  if (width <= 0 || height <= 0)
  {
    return { CommandResult::Code::InvalidArguments, "Height and width must be > 0." };
  }
  viewer.setWindowSize({ width, height });
  return { CommandResult::Code::Ok };
}


ResolutionIncrease::ResolutionIncrease()
  : Command("resolutionIncrease", Args())
{}


bool ResolutionIncrease::checkAdmissible([[maybe_unused]] Viewer &viewer) const
{
  return true;
}


CommandResult ResolutionIncrease::invoke(Viewer &viewer, [[maybe_unused]] const ExecInfo &info,
                                         [[maybe_unused]] const Args &args)
{
  const auto res = viewer.windowSize();
  const auto next = nextResolutionUp(res);
  viewer.setWindowSize(next);
  return { CommandResult::Code::Ok };
}


ResolutionDecrease::ResolutionDecrease()
  : Command("resolutionDecrease", Args())
{}


bool ResolutionDecrease::checkAdmissible([[maybe_unused]] Viewer &viewer) const
{
  return true;
}


CommandResult ResolutionDecrease::invoke(Viewer &viewer, [[maybe_unused]] const ExecInfo &info,
                                         [[maybe_unused]] const Args &args)
{
  const auto res = viewer.windowSize();
  const auto next = nextResolutionDown(res);
  viewer.setWindowSize(next);
  return { CommandResult::Code::Ok };
}
}  // namespace tes::view::command::render
