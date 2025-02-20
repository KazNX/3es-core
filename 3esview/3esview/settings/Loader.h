//
// Author: Kazys Stepanas
//
#pragma once

#include <3esview/ViewConfig.h>

#include "IOResult.h"
#include "Settings.h"

#include <filesystem>

namespace tes::view::settings
{
/// Load settings from the default settings path.
/// @param config Settings structure to load into.
/// @return Information about the success of the operation.
IOResult TES_VIEWER_API load(Settings::Config &config);
/// Load settings from the specified settings path.
/// @param config Settings structure to load into.
/// @param path The settings file path.
/// @return Information about the success of the operation.
IOResult TES_VIEWER_API load(Settings::Config &config, const std::filesystem::path &path);
/// Save settings to the default settings path.
/// @param config Settings to save.
/// @return Information about the success of the operation.
IOResult TES_VIEWER_API save(const Settings::Config &config);
/// Save settings to the specified settings path.
/// @param config Settings to save.
/// @param path The settings file path.
/// @return Information about the success of the operation.
IOResult TES_VIEWER_API save(const Settings::Config &config, const std::filesystem::path &path);
};  // namespace tes::view::settings
