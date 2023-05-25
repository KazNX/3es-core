#include "Arrow.h"

#include <3esview/mesh/Converter.h>

#include <3escore/shapes/SimpleMesh.h>
#include <3escore/tessellate/Arrow.h>

#include <mutex>

namespace tes::view::painter
{
Arrow::Arrow(const std::shared_ptr<BoundsCuller> &culler,
             const std::shared_ptr<shaders::ShaderLibrary> &shaders)
  : ShapePainter(culler, shaders, { Part{ solidMesh() } }, { Part{ wireframeMesh() } },
                 { Part{ solidMesh() } }, ShapeCache::calcSphericalBounds)
{}

Magnum::GL::Mesh Arrow::solidMesh()
{
  static SimpleMesh build_mesh(0, 0, 0, DtTriangles,
                               SimpleMesh::Vertex | SimpleMesh::Normal | SimpleMesh::Index);
  static std::mutex guard;

  const std::scoped_lock lock(guard);

  // Build with the tes tesselator.
  if (build_mesh.vertexCount() == 0)
  {
    std::vector<tes::Vector3f> vertices;
    std::vector<tes::Vector3f> normals;
    std::vector<unsigned> indices;

    constexpr unsigned kFacets = 24;
    constexpr float kHeadRadius = 1.5f;
    constexpr float kCylinderRadius = 1.0f;
    constexpr float kCylinderLength = 0.81f;
    constexpr float kArrowLength = 1.0f;
    tes::arrow::solid(vertices, indices, normals, kFacets, kHeadRadius, kCylinderRadius,
                      kCylinderLength, kArrowLength);

    build_mesh.setVertexCount(vertices.size());
    build_mesh.setIndexCount(indices.size());

    build_mesh.setVertices(0, vertices.data(), vertices.size());
    build_mesh.setNormals(0, normals.data(), normals.size());
    build_mesh.setIndices(0, indices.data(), indices.size());
  }

  return mesh::convert(build_mesh);
}


Magnum::GL::Mesh Arrow::wireframeMesh()
{
  static SimpleMesh build_mesh(0, 0, 0, DtLines, SimpleMesh::Vertex | SimpleMesh::Index);
  static std::mutex guard;

  const std::scoped_lock lock(guard);

  // Build with the tes tesselator.
  if (build_mesh.vertexCount() == 0)
  {
    std::vector<tes::Vector3f> vertices;
    std::vector<unsigned> indices;
    constexpr unsigned kFacets = 8;
    constexpr float kHeadRadius = 1.5f;
    constexpr float kCylinderRadius = 1.0f;
    constexpr float kCylinderLength = 0.81f;
    constexpr float kArrowLength = 1.0f;
    tes::arrow::wireframe(vertices, indices, kFacets, kHeadRadius, kCylinderRadius, kCylinderLength,
                          kArrowLength);

    build_mesh.setVertexCount(vertices.size());
    build_mesh.setIndexCount(indices.size());

    build_mesh.setVertices(0, vertices.data(), vertices.size());
    build_mesh.setIndices(0, indices.data(), indices.size());
  }

  return mesh::convert(build_mesh);
}
}  // namespace tes::view::painter
