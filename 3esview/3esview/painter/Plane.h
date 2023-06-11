
#ifndef TES_VIEW_PAINTER_PLANE_H
#define TES_VIEW_PAINTER_PLANE_H

#include <3esview/ViewConfig.h>

#include "ShapePainter.h"

namespace tes::view::painter
{
/// Plane painter.
class TES_VIEWER_API Plane : public ShapePainter
{
public:
  /// Constructor.
  /// @param culler Bounds culler
  Plane(const std::shared_ptr<BoundsCuller> &culler,
        const std::shared_ptr<shaders::ShaderLibrary> &shaders);

  /// Solid mesh creation function.
  /// @return A solid (or transparent) mesh representation.
  static Magnum::GL::Mesh solidMesh();

  /// Wireframe mesh creation function.
  /// @return A wireframe mesh representation.
  static Magnum::GL::Mesh wireframeMesh();

  void drawOpaque(const FrameStamp &stamp, const Magnum::Matrix4 &projection_matrix,
                  const Magnum::Matrix4 &view_matrix, const CategoryState &categories) override;
  void drawTransparent(const FrameStamp &stamp, const Magnum::Matrix4 &projection_matrix,
                       const Magnum::Matrix4 &view_matrix,
                       const CategoryState &categories) override;

private:
};
}  // namespace tes::view::painter

#endif  // TES_VIEW_PAINTER_PLANE_H
