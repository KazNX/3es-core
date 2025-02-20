#pragma once

#include "3esview/ViewConfig.h"

#include <Magnum/Magnum.h>

namespace Magnum
{
namespace GL
{
class Framebuffer;
}  // namespace GL
}  // namespace Magnum

namespace tes::view
{
/// Base class for any full screen, frame buffer object rendering effect.
class TES_VIEWER_API FboEffect
{
public:
  /// Defines the projection matrix type.
  enum class TES_VIEWER_API ProjectionType
  {
    Perspective,
    Orthographic
  };

  virtual ~FboEffect();

  /// Prepare for rendering the frame buffer effect.
  /// @param projection_matrix The projection matrix.
  /// @param projection_type Identifies the type of projection.
  /// @param near_clip The near clip plane distance.
  /// @param far_clip The far clip plane distance.
  virtual void prepareFrame(const Magnum::Matrix4 &projection_matrix,
                            ProjectionType projection_type, float near_clip, float far_clip) = 0;

  /// Complete rendering of the frame. This must blit back to the active frame buffer, normally the
  /// default.
  virtual void completeFrame() = 0;

  /// Called when the viewport changes. Allows the frame buffer to resize if required.
  /// @param viewport The new viewport dimensions.
  virtual void viewportChange(const Magnum::Range2Di &viewport) = 0;
};
}  // namespace tes::view
