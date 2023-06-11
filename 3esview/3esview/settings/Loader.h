//
// Author: Kazys Stepanas
//
#ifndef TES_VIEW_SETTINGS_LOADER_H
#define TES_VIEW_SETTINGS_LOADER_h

#include <3esview/ViewConfig.h>

#include "Settings.h"

#include <filesystem>

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

/// Load settings from the default settings path.
/// @param config Settings structure to load into.
/// @return Information about the success of the operation.
IOResult load(Settings::Config &config);
/// Load settings from the specified settings path.
/// @param config Settings structure to load into.
/// @param path The settings file path.
/// @return Information about the success of the operation.
IOResult load(Settings::Config &config, const std::filesystem::path &path);
/// Save settings to the default settings path.
/// @param config Settings to save.
/// @return Information about the success of the operation.
IOResult save(const Settings::Config &config);
/// Save settings to the specified settings path.
/// @param config Settings to save.
/// @param path The settings file path.
/// @return Information about the success of the operation.
IOResult save(const Settings::Config &config, const std::filesystem::path &path);
};  // namespace tes::view::settings

#endif  // TES_VIEW_SETTINGS_LOADER_h
