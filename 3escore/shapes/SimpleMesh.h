//
// author: Kazys Stepanas
//
#ifndef TES_CORE_SHAPES_SIMPLEMESH_H
#define TES_CORE_SHAPES_SIMPLEMESH_H

#include <3escore/CoreConfig.h>

#include "MeshComponentFlag.h"
#include "MeshResource.h"

#include <3escore/Colour.h>
#include <3escore/IntArg.h>
#include <3escore/MeshMessages.h>

#include <array>
#include <memory>

namespace tes
{
struct SimpleMeshImp;

/// An encapsulated definition of a mesh. It manages all its own vertices, indices, etc.
class TES_CORE_API SimpleMesh : public MeshResource
{
public:
  /// Construct a @c SimpleMesh resource.
  /// @param id An ID unique among all @c tes::Resource objects.
  /// @param vertex_count Number of vertices to preallocate.
  /// @param index_count Number of indices to preallocate.
  /// @param drawType Defines the primitive type being indexed.
  /// @param components The components defined by this mesh. See @c MeshComponentFlag.
  SimpleMesh(uint32_t id, size_t vertex_count = 0u, size_t index_count = 0u,
             DrawType draw_type = DrawType::Triangles,
             MeshComponentFlag components = MeshComponentFlag::Vertex | MeshComponentFlag::Index);

  /// Copy constructor supporting initial, shallow copy with copy on write semantics.
  /// @param other The mesh to copy.
  SimpleMesh(const SimpleMesh &other);

  /// Destructor.
  ~SimpleMesh() override;

  // Copy assignment is not threadsafe. Deleted.
  SimpleMesh &operator=(const SimpleMesh &other) = delete;

  /// Reset this mesh to a simple mesh with only @c Vertex and @c Index components.
  virtual void clear();

  /// Clear only the data arrays. Memory is preserved.s
  virtual void clearData();

  /// @copydoc Resource::id()
  [[nodiscard]] uint32_t id() const override;

  /// @copydoc Resource::transform()
  [[nodiscard]] Transform transform() const override;

  /// Set the object transformation matrix for this mesh.
  ///
  /// This will often be redundant when the mesh is used with a @c MeshSet object as that object
  /// defines its own object matrix and a transformation matrix for each contains @c MeshResource.
  ///
  /// @param transform The object transformation matrix for the mesh.
  void setTransform(const Transform &transform);

  /// @copydoc MeshResource::tint()
  [[nodiscard]] uint32_t tint() const override;
  /// Set the colour tint value for the mesh. The colour is defined in hex as 0xRRGGBBAA, best
  /// calculated using the
  /// @c Colour class.
  /// @param tint The RGBA tint colour.
  void setTint(uint32_t tint);

  /// Performs a shallow copy of this mesh. Note that any modification
  /// of the mesh data results in a copy of the existing data. Otherwise
  /// @c SimpleMesh objects can share their data.
  [[nodiscard]] std::shared_ptr<Resource> clone() const override;

  using MeshResource::drawType;
  /// @copydoc::MeshResource::drawType()
  [[nodiscard]] DrawType drawType(int stream) const override;

  /// Set the draw type as a @c DrawType value.
  /// @param type The draw type to set.
  void setDrawType(DrawType type);

  using MeshResource::drawScale;
  /// @copydoc::MeshResource::drawScale()
  [[nodiscard]] float drawScale(int stream) const override;

  /// Set the @c drawScale().
  /// @param scale The draw scale: must be zero or positive.
  void setDrawScale(float scale);

  /// Query the @c MeshComponentFlag components used by this mesh.
  /// @return The @c MeshComponentFlag flags.
  [[nodiscard]] MeshComponentFlag components() const;

  /// Set the @c MeshComponentFlag components for this mesh.
  ///
  /// @c MeshComponentFlag::Vertex is always implied.
  /// @param components @c MeshComponentFlag flags to set.
  void setComponents(MeshComponentFlag components);

  /// Add @c MeshComponentFlag flags to the existing set.
  /// @param components Additional @c MeshComponentFlag flags to set.
  void addComponents(MeshComponentFlag components);

  using MeshResource::vertexCount;
  [[nodiscard]] unsigned vertexCount(int stream) const override;
  void setVertexCount(size_t count);
  void reserveVertexCount(size_t count);

  unsigned addVertex(const Vector3f &v) { return addVertices(&v, 1u); }
  unsigned addVertices(const Vector3f *v, size_t count);
  inline bool setVertex(size_t at, const Vector3f &v) { return setVertices(at, &v, 1u) == 1u; }
  unsigned setVertices(size_t at, const Vector3f *v, size_t count);
  [[nodiscard]] const Vector3f *rawVertices() const;
  using MeshResource::vertices;
  [[nodiscard]] DataBuffer vertices(int stream) const override;

  using MeshResource::indexCount;
  [[nodiscard]] unsigned indexCount(int stream) const override;
  void setIndexCount(size_t count);
  void reserveIndexCount(size_t count);

  inline void addIndex(uint32_t i) { return addIndices(&i, 1u); }
  void addIndices(const uint32_t *idx, size_t count);
  inline bool setIndex(size_t at, uint32_t i) { return setIndices(at, &i, 1u) == 1u; }
  unsigned setIndices(size_t at, const uint32_t *idx, size_t count);
  [[nodiscard]] const uint32_t *rawIndices() const;
  using MeshResource::indices;
  [[nodiscard]] DataBuffer indices(int stream) const override;

  inline bool setNormal(size_t at, const Vector3f &n) { return setNormals(at, &n, 1u) == 1u; }
  unsigned setNormals(size_t at, const Vector3f *n, size_t count);
  [[nodiscard]] const Vector3f *rawNormals() const;
  using MeshResource::normals;
  [[nodiscard]] DataBuffer normals(int stream) const override;

  inline bool setColour(size_t at, uint32_t c) { return setColours(at, &c, 1u) == 1u; }
  unsigned setColours(size_t at, const uint32_t *c, size_t count);
  [[nodiscard]] const uint32_t *rawColours() const;
  using MeshResource::colours;
  [[nodiscard]] DataBuffer colours(int stream) const override;

  inline bool setUv(size_t at, float u, float v)
  {
    const std::array<float, 2> uv = { u, v };
    return setUvs(at, uv.data(), 1u) == 1u;
  }
  unsigned setUvs(size_t at, const float *uvs, size_t count);
  [[nodiscard]] const float *rawUvs() const;
  using MeshResource::uvs;
  [[nodiscard]] DataBuffer uvs(int stream) const override;

private:
  void copyOnWrite();

  bool processCreate(const MeshCreateMessage &msg, const ObjectAttributesd &attributes,
                     float draw_scale) override;
  bool processVertices(const MeshComponentMessage &msg, unsigned offset,
                       const DataBuffer &stream) override;
  bool processIndices(const MeshComponentMessage &msg, unsigned offset,
                      const DataBuffer &stream) override;
  bool processColours(const MeshComponentMessage &msg, unsigned offset,
                      const DataBuffer &stream) override;
  bool processNormals(const MeshComponentMessage &msg, unsigned offset,
                      const DataBuffer &stream) override;
  bool processUVs(const MeshComponentMessage &msg, unsigned offset,
                  const DataBuffer &stream) override;

  std::shared_ptr<SimpleMeshImp> _imp;
};


inline void SimpleMesh::addComponents(MeshComponentFlag components)
{
  setComponents(this->components() | components);
}
}  // namespace tes

#endif  // TES_CORE_SHAPES_SIMPLEMESH_H
