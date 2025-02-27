//
// Author: Kazys Stepanas
//
#include "VertexColour.h"

#include <3escore/Log.h>

#include <Magnum/GL/Mesh.h>
#include <Magnum/GL/Renderer.h>
#include <Magnum/Math/Color.h>
#include <Magnum/Math/Matrix4.h>

namespace tes::view::shaders
{
VertexColour::VertexColour()
  : _shader(std::make_shared<Magnum::Shaders::VertexColorGL3D>())
{}


VertexColour::~VertexColour() = default;


Shader &VertexColour::setProjectionMatrix(const Magnum::Matrix4 &matrix)
{
  _pvm.setProjection(matrix);
  return *this;
}


Shader &VertexColour::setViewMatrix(const Magnum::Matrix4 &matrix)
{
  _pvm.setView(matrix);
  return *this;
}


Shader &VertexColour::setModelMatrix(const Magnum::Matrix4 &matrix)
{
  _pvm.setModel(matrix);
  return *this;
}


Shader &VertexColour::setColour(const Magnum::Color4 &colour)
{
  (void)colour;
  // Not supported.
  // _shader->setColor(colour);
  return *this;
}

Shader &VertexColour::setDrawScale(float scale)
{
  Magnum::GL::Renderer::setPointSize(scale > 0 ? scale : kDefaultPointSize);
  Magnum::GL::Renderer::setLineWidth(scale > 0 ? scale : kDefaultLineWidth);
  return *this;
}

Shader &VertexColour::draw(Magnum::GL::Mesh &mesh)
{
  updateTransform();
  _shader->draw(mesh);
  return *this;
}

Shader &VertexColour::draw(Magnum::GL::Mesh &mesh, Magnum::GL::Buffer &buffer,
                           size_t instance_count)
{
  (void)mesh;
  (void)buffer;
  (void)instance_count;
  log::error("VertexColour shader does not support instanced rendering.");
  return *this;
}


void VertexColour::updateTransform()
{
  if (_pvm.dirtyPvm())
  {
    _shader->setTransformationProjectionMatrix(_pvm.pvm());
    _pvm.clearDirty();
  }
}
}  // namespace tes::view::shaders
