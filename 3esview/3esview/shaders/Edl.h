#pragma once

#include <3esview/ViewConfig.h>

#include <Magnum/GL/AbstractShaderProgram.h>
#include <Magnum/GL/Texture.h>
#include <Magnum/GL/Renderbuffer.h>
#include <Magnum/Shaders/GenericGL.h>

namespace tes::shaders
{
class TES_VIEWER_API Edl : public Magnum::GL::AbstractShaderProgram
{
public:
  using Position = Magnum::Shaders::GenericGL3D::Position;
  using TextureCoordinates = Magnum::GL::Attribute<1, Magnum::Vector2>;

  Edl();
  ~Edl();

  Edl &bindColourTexture(Magnum::GL::Texture2D &texture);

  Edl &bindDepthBuffer(Magnum::GL::Texture2D &texture);

  Edl &setProjectionMatrix(const Magnum::Matrix4 &matrix);

  /// Set the projection parameters based on the near/far clip plane distances.
  Edl &setClipParams(Magnum::Float near, Magnum::Float far, bool perspective = true,
                     bool reverse_depth = false);

  /// Set the screen/view size in pixels.
  Edl &setScreenParams(const Magnum::Vector2i &view_size);

  inline Edl &setRadius(float radius)
  {
    setUniform(_radiusUniform, radius);
    return *this;
  }

  inline Edl &setLinearScale(float linearScale)
  {
    setUniform(_linearScaleUniform, linearScale);
    return *this;
  }

  inline Edl &setExponentialScale(float exponentialScale)
  {
    setUniform(_exponentialScaleUniform, exponentialScale);
    return *this;
  }

  /// Set the simulated light direction in camera space.
  /// @param direction The direction of the light in camera space (Z forward).
  /// @return @c *this
  Edl &setLightDirection(const Magnum::Vector3 &direction);

private:
  enum : Magnum::Int
  {
    ColourUnit = 0,
    DepthUnit = 1
  };

  Magnum::Int _projectionMatrixUniform = -1;
  Magnum::Int _projectionParamsUniform = -1;
  Magnum::Int _screenParamsUniform = -1;
  Magnum::Int _radiusUniform = -1;
  Magnum::Int _linearScaleUniform = -1;
  Magnum::Int _exponentialScaleUniform = -1;
  Magnum::Int _lightDirUniform = -1;
};
}  // namespace tes::shaders
