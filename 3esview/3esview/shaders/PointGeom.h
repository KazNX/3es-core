//
// Author: Kazys Stepanas
//
#pragma once

#include <3esview/ViewConfig.h>

#include "Pvm.h"
#include "Shader.h"

#include <Magnum/GL/AbstractShaderProgram.h>
#include <Magnum/Shaders/GenericGL.h>

#include <memory>

namespace tes::view::shaders
{
class PointGeomProgram;

/// A point shader using geometry to make circular points.
class TES_VIEWER_API PointGeom : public Shader
{
public:
  /// Constructor.
  PointGeom();
  PointGeom(const PointGeom &other) = delete;
  /// Destructor.
  ~PointGeom() override;

  PointGeom &operator=(const PointGeom &other) = delete;

  [[nodiscard]] Feature features() const override
  {
    return Feature::Transparent | Feature::Tint | Feature::DrawScale;
  }

  [[nodiscard]] std::shared_ptr<Magnum::GL::AbstractShaderProgram> shader() const override;
  [[nodiscard]] std::shared_ptr<PointGeomProgram> typedShader() const { return _shader; }

  Shader &setProjectionMatrix(const Magnum::Matrix4 &matrix) override;
  Shader &setViewMatrix(const Magnum::Matrix4 &matrix) override;
  Shader &setModelMatrix(const Magnum::Matrix4 &matrix) override;

  Shader &setViewportSize(const Magnum::Vector2i &size) override;

  Shader &setColour(const Magnum::Color4 &colour) override;

  Shader &setDrawScale(float scale) override;

  Shader &draw(Magnum::GL::Mesh &mesh) override;
  Shader &draw(Magnum::GL::Mesh &mesh, Magnum::GL::Buffer &buffer, size_t instance_count) override;

private:
  void updateTransform();

  /// Internal shader.
  std::shared_ptr<PointGeomProgram> _shader;
  Pvm _pvm;
};

/// The underlying Magnum shader implementation for point rendering.
class TES_VIEWER_API PointGeomProgram : public Magnum::GL::AbstractShaderProgram
{
public:
  using Generic = Magnum::Shaders::GenericGL3D;

  using Position = Generic::Position;
  using Color3 = Generic::Color3;
  using Color4 = Generic::Color4;
  using Int = Magnum::Int;

  enum : Magnum::UnsignedInt
  {
    /**
     * Color shader output. @ref shaders-generic "Generic output",
     * present always. Expects three- or four-component floating-point
     * or normalized buffer attachment.
     */
    ColorOutput = Generic::ColorOutput
  };

  explicit PointGeomProgram();
  explicit PointGeomProgram([[maybe_unused]] Magnum::NoCreateT no_create) noexcept
    : AbstractShaderProgram(Magnum::NoCreate)
  {}

  PointGeomProgram(const PointGeomProgram &) = delete;
  PointGeomProgram(PointGeomProgram &&) noexcept = default;
  PointGeomProgram &operator=(const PointGeomProgram &) = delete;
  PointGeomProgram &operator=(PointGeomProgram &&) noexcept = default;

  ~PointGeomProgram() override = default;

  /// Set just the projection matrix.
  /// @param matrix
  /// @return
  PointGeomProgram &setProjectionMatrix(const Magnum::Matrix4 &matrix);
  /// Set the view * model matrix.
  /// @param matrix
  /// @return
  PointGeomProgram &setViewModelTransform(const Magnum::Matrix4 &matrix);

  PointGeomProgram &setTint(const Magnum::Color4 &colour);

  /// Set the point rendering size.
  /// @param size
  /// @return
  PointGeomProgram &setPointSize(float size);

  PointGeomProgram &setViewportSize(const Magnum::Vector2i &size);

private:
  Int _view_model_matrix_uniform = 0;
  Int _tint_uniform = 1;
  Int _projection_matrix_uniform = 2;
  Int _screen_params_uniform = 3;
  Int _point_size_uniform = 4;
};
}  // namespace tes::view::shaders
