//
// Author: Kazys Stepanas
//
#ifndef TES_VIEW_SHADERS_FLAT_H
#define TES_VIEW_SHADERS_FLAT_H

#include <3esview/ViewConfig.h>

#include "Shader.h"
#include "Pvm.h"

#include <Magnum/Shaders/Flat.h>

#include <memory>

namespace tes::view::shaders
{
/// Flat colour shader. Can be used for solid, transparent and line based shapes and supports
/// instance rendering.
class TES_VIEWER_API Flat : public Shader
{
public:
  /// Constructor.
  Flat();
  /// Destructor.
  ~Flat();

  Feature features() const override
  {
    return Feature::Instance | Feature::Transparent | Feature::Tint;
  }

  std::shared_ptr<Magnum::GL::AbstractShaderProgram> shader() const override { return _shader; }
  std::shared_ptr<Magnum::Shaders::Flat3D> typedShader() const { return _shader; }

  Shader &setProjectionMatrix(const Magnum::Matrix4 &matrix) override;
  Shader &setViewMatrix(const Magnum::Matrix4 &matrix) override;
  Shader &setModelMatrix(const Magnum::Matrix4 &matrix) override;

  Shader &setColour(const Magnum::Color4 &colour) override;

  Shader &setDrawScale(float scale) override
  {
    (void)scale;
    return *this;
  }

  Shader &draw(Magnum::GL::Mesh &mesh) override;
  Shader &draw(Magnum::GL::Mesh &mesh, Magnum::GL::Buffer &buffer, size_t instance_count) override;

private:
  void updateTransform();

  /// Internal shader.
  std::shared_ptr<Magnum::Shaders::Flat3D> _shader;
  Pvm _pvm;
};

}  // namespace tes::view::shaders

#endif  // TES_VIEW_SHADERS_FLAT_H
