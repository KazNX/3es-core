//
// author: Kazys Stepanas
//
#pragma once

#include "CoreConfig.h"

namespace tes
{
/// Target compression levels.
enum class CompressionLevel : uint16_t
{
  None,
  Low,
  Medium,
  High,
  VeryHigh,

  Levels,

  Default = Medium
};
}  // namespace tes
