#include "Cone.h"

#include <3esview/mesh/Converter.h>

#include <3escore/shapes/SimpleMesh.h>
#include <3escore/tessellate/Cone.h>

#include <mutex>

namespace tes::view::painter
{
Cone::Cone(const std::shared_ptr<BoundsCuller> &culler,
           const std::shared_ptr<shaders::ShaderLibrary> &shaders)
  : ShapePainter(culler, shaders, { Part{ solidMesh() } }, { Part{ wireframeMesh() } },
                 { Part{ solidMesh() } }, ShapeCache::calcSphericalBounds)
{}

Magnum::GL::Mesh Cone::solidMesh()
{
  static SimpleMesh build_mesh(
    0, 0, 0, DrawType::Triangles,
    MeshComponentFlag::Vertex | MeshComponentFlag::Normal | MeshComponentFlag::Index);
  static std::mutex guard;

  const std::scoped_lock lock(guard);

  // Build with the tes tesselator.
  if (build_mesh.vertexCount() == 0)
  {
    // Calculate the cone radius from the cone angle.
    //        /|
    //       /a|
    //      /  |
    //     /   | h
    //    /    |
    //   /     |
    //    -----
    //      r
    // a = atan(r/h)
    // r = h * tan(a)
    constexpr float kConeLength = 1.0f;
    constexpr float kConeRadius = 1.0f;
    const float cone_angle = std::atan(kConeRadius / kConeLength);
    constexpr unsigned kFacets = 24;

    std::vector<tes::Vector3f> vertices;
    std::vector<tes::Vector3f> normals;
    std::vector<unsigned> indices;
    tes::cone::solid(vertices, indices, normals, Vector3f(0, 0, kConeLength),
                     Vector3f(0, 0, kConeLength), kConeLength, cone_angle, kFacets);

    build_mesh.setVertexCount(vertices.size());
    build_mesh.setIndexCount(indices.size());

    build_mesh.setVertices(0, vertices.data(), vertices.size());
    build_mesh.setNormals(0, normals.data(), normals.size());
    build_mesh.setIndices(0, indices.data(), indices.size());
  }

  return mesh::convert(build_mesh);
}


Magnum::GL::Mesh Cone::wireframeMesh()
{
  static SimpleMesh build_mesh(0, 0, 0, DrawType::Lines,
                               MeshComponentFlag::Vertex | MeshComponentFlag::Index);
  static std::mutex guard;

  const std::scoped_lock lock(guard);

  // Build with the tes tesselator.
  if (build_mesh.vertexCount() == 0)
  {
    // Calculate the cone radius from the cone angle.
    //        /|
    //       /a|
    //      /  |
    //     /   | h
    //    /    |
    //   /     |
    //    -----
    //      r
    // a = atan(r/h)
    // r = h * tan(a)
    constexpr float kConeLength = 1.0f;
    constexpr float kConeRadius = 1.0f;
    const float cone_angle = std::atan(kConeRadius / kConeLength);
    constexpr unsigned kFacets = 16;
    std::vector<tes::Vector3f> vertices;
    std::vector<unsigned> indices;
    tes::cone::wireframe(vertices, indices, Vector3f(0, 0, kConeLength),
                         Vector3f(0, 0, kConeLength), kConeLength, cone_angle, kFacets);

    build_mesh.setVertexCount(vertices.size());
    build_mesh.setIndexCount(indices.size());

    build_mesh.setVertices(0, vertices.data(), vertices.size());
    build_mesh.setIndices(0, indices.data(), indices.size());
  }

  return mesh::convert(build_mesh);
}
}  // namespace tes::view::painter
