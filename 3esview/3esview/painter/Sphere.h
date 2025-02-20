#pragma once

#include <3esview/ViewConfig.h>

#include "ShapePainter.h"

namespace tes::view::painter
{
/// Sphere painter.
class TES_VIEWER_API Sphere : public ShapePainter
{
public:
  /// Constructor.
  /// @param culler Bounds culler
  Sphere(const std::shared_ptr<BoundsCuller> &culler,
         const std::shared_ptr<shaders::ShaderLibrary> &shaders);

  /// Solid mesh creation function.
  /// @return A solid (or transparent) mesh representation.
  static Magnum::GL::Mesh solidMesh();

  /// Wireframe mesh creation function.
  /// @return A wireframe mesh representation.
  static Magnum::GL::Mesh wireframeMesh();

private:
};
}  // namespace tes::view::painter
