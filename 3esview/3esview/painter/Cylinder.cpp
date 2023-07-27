#include "Cylinder.h"

#include <3esview/mesh/Converter.h>

#include <3escore/shapes/SimpleMesh.h>
#include <3escore/tessellate/Cylinder.h>

#include <mutex>

namespace tes::view::painter
{
Cylinder::Cylinder(const std::shared_ptr<BoundsCuller> &culler,
                   const std::shared_ptr<shaders::ShaderLibrary> &shaders)
  : ShapePainter(culler, shaders, { Part{ solidMesh() } }, { Part{ wireframeMesh() } },
                 { Part{ solidMesh() } }, calculateBounds)
{}


void Cylinder::calculateBounds(const Magnum::Matrix4 &transform, Bounds &bounds)
{
  return ShapeCache::calcCylindricalBounds(transform, 1.0f, 1.0f, bounds);
}


Magnum::GL::Mesh Cylinder::solidMesh()
{
  static SimpleMesh build_mesh(
    0, 0, 0, DrawType::Triangles,
    MeshComponentFlag::Vertex | MeshComponentFlag::Normal | MeshComponentFlag::Index);
  static std::mutex guard;

  const std::scoped_lock lock(guard);

  // Build with the tes tesselator.
  if (build_mesh.vertexCount() == 0)
  {
    std::vector<tes::Vector3f> vertices;
    std::vector<tes::Vector3f> normals;
    std::vector<unsigned> indices;

    constexpr Vector3f kAxis = { 0, 0, 1 };
    constexpr float kHeight = 1.0f;
    constexpr float kRadius = 1.0f;
    constexpr unsigned kFacets = 24;
    tes::cylinder::solid(vertices, indices, normals, kAxis, kHeight, kRadius, kFacets);

    build_mesh.setVertexCount(vertices.size());
    build_mesh.setIndexCount(indices.size());

    build_mesh.setVertices(0, vertices.data(), vertices.size());
    build_mesh.setNormals(0, normals.data(), normals.size());
    build_mesh.setIndices(0, indices.data(), indices.size());
  }

  return mesh::convert(build_mesh);
}


Magnum::GL::Mesh Cylinder::wireframeMesh()
{
  static SimpleMesh build_mesh(0, 0, 0, DrawType::Lines,
                               MeshComponentFlag::Vertex | MeshComponentFlag::Index);
  static std::mutex guard;

  const std::scoped_lock lock(guard);

  // Build with the tes tesselator.
  if (build_mesh.vertexCount() == 0)
  {
    std::vector<tes::Vector3f> vertices;
    std::vector<unsigned> indices;

    constexpr Vector3f kAxis = { 0, 0, 1 };
    constexpr float kHeight = 1.0f;
    constexpr float kRadius = 1.0f;
    constexpr unsigned kFacets = 8;
    tes::cylinder::wireframe(vertices, indices, kAxis, kHeight, kRadius, kFacets);

    build_mesh.setVertexCount(vertices.size());
    build_mesh.setIndexCount(indices.size());

    build_mesh.setVertices(0, vertices.data(), vertices.size());
    build_mesh.setIndices(0, indices.data(), indices.size());
  }

  return mesh::convert(build_mesh);
}
}  // namespace tes::view::painter
