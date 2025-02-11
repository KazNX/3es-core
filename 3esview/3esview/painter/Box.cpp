#include "Box.h"

#include <3esview/mesh/Converter.h>

#include <3escore/shapes/SimpleMesh.h>
#include <3escore/tessellate/Box.h>

#include <mutex>

namespace tes::view::painter
{
Box::Box(const std::shared_ptr<BoundsCuller> &culler,
         const std::shared_ptr<shaders::ShaderLibrary> &shaders)
  : ShapePainter(culler, shaders, { Part{ solidMesh() } }, { Part{ wireframeMesh() } },
                 { Part{ solidMesh() } }, ShapeCache::calcSphericalBounds)
{}

Magnum::GL::Mesh Box::solidMesh()
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
    tes::box::solid(vertices, indices, normals, Vector3f(1.0f));

    build_mesh.setVertexCount(vertices.size());
    build_mesh.setIndexCount(indices.size());

    build_mesh.setVertices(0, vertices.data(), vertices.size());
    build_mesh.setNormals(0, normals.data(), normals.size());
    build_mesh.setIndices(0, indices.data(), indices.size());
  }

  return mesh::convert(build_mesh);
}


Magnum::GL::Mesh Box::wireframeMesh()
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
    tes::box::wireframe(vertices, indices, Vector3f(1.0f));

    build_mesh.setVertexCount(vertices.size());
    build_mesh.setIndexCount(indices.size());

    build_mesh.setVertices(0, vertices.data(), vertices.size());
    build_mesh.setIndices(0, indices.data(), indices.size());
  }

  return mesh::convert(build_mesh);
}
}  // namespace tes::view::painter
