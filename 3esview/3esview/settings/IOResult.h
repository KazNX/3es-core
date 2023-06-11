//
// Author: Kazys Stepanas
//
#ifndef TES_VIEW_SETTINGS_IO_RESULT_H
#define TES_VIEW_SETTINGS_IO_RESULT_H

#include <3esview/ViewConfig.h>

#include <string>

namespace tes::view::settings
{
/// Result code for settings IO operations.
enum class IOCode : unsigned
{
  /// Settings serialised OK.
  Ok,
  /// Partial load or save success.
  Partial,
  /// An error occurred preventing any settings from being saved or loaded.
  Error
};

/// Result structure for settings IO operations.
struct IOResult
{
  /// Result code.
  IOCode code;
  /// Information message (e.g., on error).
  std::string message;
};
}  // namespace tes::view::settings

#endif  // TES_VIEW_SETTINGS_IO_RESULT_H