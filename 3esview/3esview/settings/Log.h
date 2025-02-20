//
// Author: Kazys Stepanas
//
#pragma once

#include <3esview/ViewConfig.h>

#include "Values.h"

namespace tes::view::settings
{
// NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers)
struct Log
{
  UInt log_history = { "Log history", 10000, 0, 1000000, "Size of the log history." };

  [[nodiscard]] inline bool operator==(const Log &other) const
  {
    return log_history == other.log_history;
  }

  [[nodiscard]] inline bool operator!=(const Log &other) const { return !operator==(other); }
};
// NOLINTEND(cppcoreguidelines-avoid-magic-numbers)
}  // namespace tes::view::settings
