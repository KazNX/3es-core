//
// author: Kazys Stepanas
//
#ifndef TES_CORE_SHAPES_MESH_SHAPE_H
#define TES_CORE_SHAPES_MESH_SHAPE_H

#include <3escore/CoreConfig.h>

#include "MeshResource.h"
#include "Shape.h"

//
#include <3escore/DataBuffer.h>
#include <3escore/Debug.h>
#include <3escore/MeshMessages.h>

#include <utility>
#include <vector>

namespace tes
{
/// A @c Shape which uses vertices and indices to render.
///
/// @c Use @c MeshSet for large data sets.
class TES_CORE_API MeshShape final : public Shape
{
public:
  /// Provides a @c MeshResource wrapper around a @c MeshShape object. This
  /// allows the details of the @c MeshShape to be read via the @c MeshResource
  /// API.
  class TES_CORE_API Resource final : public MeshResource
  {
  public:
    /// Wraps a @c MeshShape with a @c MeshResource.
    ///
    /// The user must supply a @p resource_id since @c MeshResource should have a unique @c id(),
    /// but we cannot infer this from this from the @p shape. An arbitrary value, such as zero,
    /// is fine so long as the resource is not transferred over a @c Connection.
    ///
    /// @param shape The @c MeshShape to wrap.
    /// @param resource_id A user supplied resource id. Must be unique if the @c Resource is to
    /// be sent over a @c Connection.
    Resource(MeshShape &shape, uint32_t resource_id);
    Resource(const Resource &other) = delete;
    Resource(Resource &&other) noexcept = delete;

    ~Resource() final = default;

    Resource &operator=(const Resource &other) = delete;
    Resource &operator=(Resource &&other) noexcept = delete;

    [[nodiscard]] uint32_t id() const final;

    [[nodiscard]] std::shared_ptr<tes::Resource> clone() const final;

    [[nodiscard]] Transform transform() const final;
    [[nodiscard]] uint32_t tint() const final;
    [[nodiscard]] uint8_t drawType(int stream) const final;
    using MeshResource::drawType;
    [[nodiscard]] float drawScale(int stream) const final;
    using MeshResource::drawScale;
    [[nodiscard]] unsigned vertexCount(int stream) const final;
    using MeshResource::vertexCount;
    [[nodiscard]] unsigned indexCount(int stream) const final;
    using MeshResource::indexCount;
    [[nodiscard]] DataBuffer vertices(int stream) const final;
    using MeshResource::vertices;
    [[nodiscard]] DataBuffer indices(int stream) const final;
    using MeshResource::indices;
    [[nodiscard]] DataBuffer normals(int stream) const final;
    using MeshResource::normals;
    [[nodiscard]] DataBuffer uvs(int stream) const final;
    using MeshResource::uvs;
    [[nodiscard]] DataBuffer colours(int stream) const final;
    using MeshResource::colours;

    /// Not allowed.
    /// @param packet Ignored.
    /// @return @c false
    bool readCreate(PacketReader &packet) final;
    /// Not allowed.
    /// @param message_type Ignored.
    /// @param packet Ignored.
    /// @return @c false
    bool readTransfer(int message_type, PacketReader &packet) final;

  private:
    MeshShape &_shape;
    uint32_t _resource_id = 0;
  };

  /// Codes for @c writeData().
  ///
  /// The @c SDTEnd flag is send in the send data value on the last message. However, the flag
  /// was a retrofit, so the @c SDT_ExpectEnd flag must also be set in every data send message
  /// so the receiver knows to look for the end flag.
  ///
  /// Without the @c SDT_ExpectEnd flag, the receiver assumes finalisation when all vertices
  /// and indices have been read. Other data such as normals may be written before vertices
  /// and indices, but this is an optional send implementation.
  ///
  /// For @c DtPoints , the points are coloured by type when the colour value is zero (black, with
  /// zero alpha). This is the default colour for @c DtPoints.
  ///
  /// Note: normals must be sent before completing vertices and indices. Best done first.
  enum SendDataType : uint16_t
  {
    SDTVertices,
    SDTIndices,
    SDTNormals,
    SDTColours,

    /// Last send message?
    SDTEnd = 0xffffu
  };

  MeshShape();

  /// Persistent triangle constructor accepting an iterator and optional positioning.
  /// @param vertices Pointer to the vertex array. Must be at least 3 elements per vertex.
  /// @param vertexCount The number of vertices in @c vertices.
  /// @param vertexByteSize The size of a single vertex in @p vertices. Must be at least three
  /// floats (12).
  /// @param id Unique ID for the triangles. Must be non-zero to be persistent.
  /// @param position Local to world positioning of the triangles. Defaults to the origin.
  /// @param rotation Local to world rotation of the triangles. Defaults to identity.
  /// @param scale Scaling for the triangles. Defaults to one.
  MeshShape(DrawType draw_type, const Id &id, DataBuffer vertices = {},
            const Transform &transform = Transform());

  /// Persistent triangle constructor accepting vertex and triangle iterators and optional
  /// positioning.
  /// @param vertices Pointer to the vertex array. Must be at least 3 elements per vertex.
  /// @param vertexCount The number of vertices in @c vertices.
  /// @param vertexByteSize The size of a single vertex in @p vertices. Must be at least three
  /// floats (12).
  /// @param id Unique ID for the triangles. Must be non-zero to be persistent.
  /// @param position Local to world positioning of the triangles. Defaults to the origin.
  /// @param rotation Local to world rotation of the triangles. Defaults to identity.
  /// @param scale Scaling for the triangles. Defaults to one.
  MeshShape(DrawType draw_type, const Id &id, DataBuffer vertices, DataBuffer indices,
            const Transform &transform = Transform());

  /// Copy constructor.
  /// @param other Object to copy.
  MeshShape(const MeshShape &other);

  /// Move constructor.
  /// @param other Object to move.
  MeshShape(MeshShape &&other) noexcept;

  /// Destructor.
  ~MeshShape() override;

  /// Copy assignment.
  /// @param other Object to copy.
  MeshShape &operator=(const MeshShape &other);

  /// Move assignment.
  /// @param other Object to move.
  MeshShape &operator=(MeshShape &&other) noexcept;

  [[nodiscard]] const char *type() const override { return "meshShape"; }

  /// Mark as complex to ensure @c writeData() is called.
  [[nodiscard]] bool isComplex() const override { return true; }

  /// Calculate vertex normals in the viewer?
  [[nodiscard]] bool calculateNormals() const;
  /// Should normals be calculated for the mesh by the viewer?
  /// @param calculate True to calculate vertex normals in the viewer.
  MeshShape &setCalculateNormals(bool calculate);

  /// Colour @c DtPoints by height. Requires @c drawType() @c DtPoints .
  ///
  /// This sets the shape colour to zero (black, with zero alpha).
  ///
  /// Ignored for non point types.
  /// @param colour_by_height True to colour by height.
  /// @return @c *this
  MeshShape &setColourByHeight(bool colour_by_height);

  /// Check if colouring points by height. Requires @c drawType() @c DtPoints .
  /// @return True if the @c drawType() is @c DtPoints and set to colour by height.
  [[nodiscard]] bool colourByHeight() const;

  /// Set the draw scale used to (de)emphasise the rendering.
  ///
  /// This equates to point size for @c DtPoints or line width for @c DtLines.
  /// A zero value indicates use of the viewer default drawing scale.
  ///
  /// The viewer is free to ignore this value.
  ///
  /// @param scale The new draw value. Zero or one for default.
  void setDrawScale(float scale);
  /// Get the draw scale.
  /// @return The draw scale.
  [[nodiscard]] float drawScale() const;

  /// Set (optional) mesh normals. The number of normal elements in @p normals
  /// must match the @p vertexCount.
  ///
  /// The normals array is copied if this object owns its vertex memory such
  /// as after calling @p expandVertices().
  ///
  /// Sets @c calculateNormals() to false.
  ///
  /// @param normals The normals array.
  /// @param normalByteSize the number of bytes between each element of @p normals.
  /// @return this
  MeshShape &setNormals(const DataBuffer &normals);

  /// Sets a single normal to be shared by all vertices in the mesh.
  /// Sets @c calculateNormals() to false.
  /// @param normal The shared normal to set.
  /// @return this
  MeshShape &setUniformNormal(const Vector3f &normal);

  /// Set the colours array, one per vertex (@c vertexCount()). The array pointer is borrowed if
  /// this shape doesn't own its own pointers, otherwise it is copied. Should be called before
  /// calling @c expandVertices().
  ///
  /// For @c DtPoints , this also clears @c setColourByHeight().
  ///
  /// @param colours The colours array.
  MeshShape &setColours(DataBuffer colour_buffer)
  {
    if (colour_buffer.count() >= vertices().count())
    {
      setColourByHeight(false);
      _colours = std::move(colour_buffer);
    }
    return *this;
  }

  /// Expand the vertex set into a new block of memory.
  ///
  /// This is useful when indexing small primitive from a large set of vertices.
  /// The method allocates a new array of vertices, explicitly copying and unpacking
  /// the vertices by traversing the index array. This ensure only the indexed
  /// subset is present.
  ///
  /// Invokes @c duplicateArrays() when the shape does not use indices.
  /// @return this
  MeshShape &expandVertices();

  /// Duplicate internal arrays and take ownership of the memory.
  /// Does nothing if already owning the memory.
  /// @return this
  MeshShape &duplicateArrays();

  /// Access the vertices as a @c DataBuffer . The underlying pointer type must be either @c float
  /// or @c double .
  /// @return The vertices vertex stream.
  [[nodiscard]] const DataBuffer &vertices() const { return _vertices; }
  /// Access the normals as a @c DataBuffer . The underlying pointer type must be either @c float or
  /// @c double .
  ///
  /// When non-null, the count may either match the @c vertices() count or 1, indicating a single
  /// normal for all vertices.
  ///
  /// @return The normals vertex stream.
  [[nodiscard]] const DataBuffer &normals() const { return _normals; }
  [[nodiscard]] const DataBuffer &indices() const { return _indices; }
  [[nodiscard]] const DataBuffer &colours() const { return _colours; }
  [[nodiscard]] DrawType drawType() const { return _draw_type; }

  /// Writes the standard create message and appends mesh data.
  ///
  /// - Vertex count : uint32
  /// - Index count : uint32
  /// - Draw type : uint8
  /// @param stream The stream to write to.
  /// @return True on success.
  bool writeCreate(PacketWriter &packet) const override;
  int writeData(PacketWriter &packet, unsigned &progress_marker) const override;

  bool readCreate(PacketReader &packet) override;
  bool readData(PacketReader &packet) override;

  /// Deep copy clone.
  /// @return A deep copy.
  [[nodiscard]] std::shared_ptr<Shape> clone() const override;

protected:
  void onClone(MeshShape &copy) const;

private:
  DataBuffer _vertices;  ///< Mesh vertices.
  /// Normal stream. Expect zero, one per vertex or one to apply to all vertices.
  DataBuffer _normals;
  DataBuffer _colours;              ///< Per vertex colours. Null for none.
  DataBuffer _indices;              ///< Per vertex colours. Null for none.
  double _quantisation_unit = 0.0;  ///< Quantisation for data packing. Zero => no packing.
  float _draw_scale = 0.0f;         ///< Draw scale: point scaling, line width, etc.
  DrawType _draw_type = DtPoints;   ///< The primitive to render.
};


inline MeshShape::MeshShape()
  : Shape(SIdMeshShape)
  , _draw_type(DtTriangles)
{}


inline MeshShape::MeshShape(DrawType draw_type, const Id &id, DataBuffer vertices,
                            const Transform &transform)
  : Shape(SIdMeshShape, id, transform)
  , _vertices(std::move(vertices))
  , _draw_type(draw_type)
{
  if (draw_type == DtPoints)
  {
    setColourByHeight(true);
  }
}


inline MeshShape::MeshShape(DrawType draw_type, const Id &id, DataBuffer vertices,
                            DataBuffer indices, const Transform &transform)
  : Shape(SIdMeshShape, id, transform)
  , _vertices(std::move(vertices))
  , _indices(std::move(indices))
  , _draw_type(draw_type)
{
  // NOLINTNEXTLINE(readability-simplify-boolean-expr)
  TES_ASSERT2(_indices.type() == DctInt8 || _indices.type() == DctInt16 ||
                _indices.type() == DctInt32 || _indices.type() == DctUInt8 ||
                _indices.type() == DctUInt16 || _indices.type() == DctUInt32,
              "Indices not of integery type (4-byte width limit)")
  if (draw_type == DtPoints)
  {
    setColourByHeight(true);
  }
}


inline bool MeshShape::calculateNormals() const
{
  return (flags() & MeshShapeCalculateNormals) != 0;
}


inline MeshShape &MeshShape::setCalculateNormals(bool calculate)
{
  auto new_flags = static_cast<uint16_t>(flags() & ~MeshShapeCalculateNormals);
  new_flags |= MeshShapeCalculateNormals * !!calculate;
  setFlags(new_flags);
  return *this;
}


inline MeshShape &MeshShape::setColourByHeight(bool colour_by_height)
{
  if (drawType() == DtPoints)
  {
    if (colour_by_height)
    {
      setColour(Colour(0));
    }
    else if (colour().colour32() == 0)
    {
      setColour(Colour(0xFFFFFFFFu));
    }
  }

  return *this;
}

inline bool MeshShape::colourByHeight() const
{
  return drawType() == DtPoints && colour().colour32() == 0;
}


inline void MeshShape::setDrawScale(float scale)
{
  _draw_scale = scale;
}


inline float MeshShape::drawScale() const
{
  return _draw_scale;
}
}  // namespace tes

#endif  // TES_CORE_SHAPES_MESH_SHAPE_H
