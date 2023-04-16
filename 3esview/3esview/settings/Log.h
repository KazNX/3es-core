//
// Author: Kazys Stepanas
//
#ifndef TES_VIEW_SETTINGS_LOG_H
#define TES_VIEW_SETTINGS_LOG_H

#include <3esview/ViewConfig.h>

#include "Values.h"

namespace tes::view::settings
{
struct Log
{
  UInt log_history = { "Log history", 10000, 0, 1000000, "Size of the log history." };

  [[nodiscard]] inline bool operator==(const Log &other) const
  {
    return log_history == other.log_history;
  }

  [[nodiscard]] inline bool operator!=(const Log &other) const { return !operator==(other); }
};
}  // namespace tes::view::settings

#endif  // TES_VIEW_SETTINGS_LOG_H
