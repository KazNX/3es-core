#include "Star.h"

#include <3esview/mesh/Converter.h>

#include <3escore/shapes/SimpleMesh.h>

#include <Magnum/GL/Renderer.h>

#include <mutex>

namespace tes::view::painter
{
Star::Star(const std::shared_ptr<BoundsCuller> &culler,
           const std::shared_ptr<shaders::ShaderLibrary> &shaders)
  : ShapePainter(culler, shaders, { Part{ solidMesh() } }, { Part{ wireframeMesh() } },
                 { Part{ solidMesh() } }, ShapeCache::calcSphericalBounds)
{}

Magnum::GL::Mesh Star::solidMesh()
{
  static SimpleMesh build_mesh(
    0, 0, 0, DrawType::Triangles,
    MeshComponentFlag::Vertex | MeshComponentFlag::Normal | MeshComponentFlag::Index);
  static std::mutex guard;

  const std::scoped_lock lock(guard);

  // Build with the tes tesselator.
  if (build_mesh.vertexCount() == 0)
  {
    const std::array<tes::Vector3f, 18> vertices = {
      Vector3f(-0.2f, 0, 0), Vector3f(0.2f, 0, 0),  //
      Vector3f(0, -0.2f, 0), Vector3f(0, 0.2f, 0),  //
      Vector3f(0, 0, 1),     Vector3f(0, 0, -1),    //
      Vector3f(0, -0.2f, 0), Vector3f(0, 0.2f, 0),  //
      Vector3f(0, 0, -0.2f), Vector3f(0, 0, 0.2f),  //
      Vector3f(1, 0, 0),     Vector3f(-1, 0, 0),    //
      Vector3f(-0.2f, 0, 0), Vector3f(0.2f, 0, 0),  //
      Vector3f(0, 0, -0.2f), Vector3f(0, 0, 0.2f),  //
      Vector3f(0, 1, 0),     Vector3f(0, -1, 0),    //
    };
    const std::array<unsigned, 36> indices = { 0,  1,  4,  0,  1,  5,  2,  3,  4,  2,  3,  5,
                                               6,  7,  10, 6,  7,  11, 8,  9,  10, 8,  9,  11,
                                               12, 13, 16, 12, 13, 17, 14, 15, 16, 14, 15, 17 };
    const std::vector<tes::Vector3f> normals(vertices.size(), Vector3f(0, 0, 1));

    build_mesh.setVertexCount(vertices.size());
    build_mesh.setIndexCount(indices.size());

    build_mesh.setVertices(0, vertices.data(), vertices.size());
    build_mesh.setNormals(0, normals.data(), normals.size());
    build_mesh.setIndices(0, indices.data(), indices.size());
  }

  return mesh::convert(build_mesh);
}


Magnum::GL::Mesh Star::wireframeMesh()
{
  static SimpleMesh build_mesh(0, 0, 0, DrawType::Lines,
                               MeshComponentFlag::Vertex | MeshComponentFlag::Index);
  static std::mutex guard;

  const std::scoped_lock lock(guard);

  // Build with the tes tesselator.
  if (build_mesh.vertexCount() == 0)
  {
    const std::array<tes::Vector3f, 6> vertices = {
      Vector3f(-1, 0, 0), Vector3f(1, 0, 0),  Vector3f(0, -1, 0),
      Vector3f(0, 1, 0),  Vector3f(0, 0, -1), Vector3f(0, 0, 1),
    };
    const std::array<unsigned, 6> indices = { 0, 1, 2, 3, 4, 5 };

    build_mesh.setVertexCount(vertices.size());
    build_mesh.setIndexCount(indices.size());

    build_mesh.setVertices(0, vertices.data(), vertices.size());
    build_mesh.setIndices(0, indices.data(), indices.size());
  }

  return mesh::convert(build_mesh);
}


void Star::drawOpaque(const FrameStamp &stamp, const Magnum::Matrix4 &projection_matrix,
                      const Magnum::Matrix4 &view_matrix, const CategoryState &categories)
{
  Magnum::GL::Renderer::disable(Magnum::GL::Renderer::Feature::FaceCulling);
  ShapePainter::drawOpaque(stamp, projection_matrix, view_matrix, categories);
  Magnum::GL::Renderer::enable(Magnum::GL::Renderer::Feature::FaceCulling);
}


void Star::drawTransparent(const FrameStamp &stamp, const Magnum::Matrix4 &projection_matrix,
                           const Magnum::Matrix4 &view_matrix, const CategoryState &categories)
{
  Magnum::GL::Renderer::disable(Magnum::GL::Renderer::Feature::FaceCulling);
  ShapePainter::drawTransparent(stamp, projection_matrix, view_matrix, categories);
  Magnum::GL::Renderer::enable(Magnum::GL::Renderer::Feature::FaceCulling);
}
}  // namespace tes::view::painter
