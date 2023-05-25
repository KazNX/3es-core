//
// Author: Kazys Stepanas
//
#ifndef TES_VIEW_SETTINGS_CONNECTION_H
#define TES_VIEW_SETTINGS_CONNECTION_H

#include <3esview/ViewConfig.h>

#include <string>
#include <vector>

namespace tes::view::settings
{
// NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers)
struct Connection
{
  /// Max items for history.
  constexpr static int kHistoryLimit = 16;

  /// Connection history. Not displayable as a property, but does need serialisation.
  /// Requires special handling.
  std::vector<std::pair<std::string, uint16_t>> history;

  [[nodiscard]] inline bool operator==(const Connection &other) const
  {
    return history == other.history;
  }

  [[nodiscard]] inline bool operator!=(const Connection &other) const { return !operator==(other); }
};
// NOLINTEND(cppcoreguidelines-avoid-magic-numbers)
}  // namespace tes::view::settings

#endif  // TES_VIEW_SETTINGS_CONNECTION_H
