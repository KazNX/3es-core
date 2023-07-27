//
// author: Kazys Stepanas
//
#ifndef TES_CORE_COMPRESSION_LEVEL_H
#define TES_CORE_COMPRESSION_LEVEL_H

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

#endif  // TES_CORE_COMPRESSION_LEVEL_H
