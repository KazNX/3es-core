//
// author: Kazys Stepanas
//
#pragma once

#include <3escore/CoreConfig.h>

#include <3escore/Enum.h>

namespace tes
{
/// Flags indicating which components of a mesh are present.
enum class MeshComponentFlag
{
  Zero = 0u,
  Vertex = (1u << 0u),  ///< Contains vertices. This flag is enforced.
  Index = (1u << 1u),
  Colour = (1u << 2u),
  Color = Colour,
  Normal = (1u << 3u),
  Uv = (1u << 4u)
};

TES_ENUM_FLAGS(MeshComponentFlag);
}  // namespace tes
