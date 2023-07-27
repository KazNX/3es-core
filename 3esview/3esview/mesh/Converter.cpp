#include "Converter.h"

#include <3esview/MagnumColour.h>

#include <3escore/MeshMessages.h>
#include <3escore/shapes/MeshResource.h>

#include <Magnum/Magnum.h>

#include <Magnum/Math/Color.h>
#include <Magnum/Math/Vector3.h>
#include <Magnum/MeshTools/Compile.h>
#include <Magnum/Trade/MeshData.h>

#include <Corrade/Containers/Array.h>
#include <Corrade/Containers/ArrayViewStl.h>

#include <vector>

namespace tes::view::mesh
{
struct VertexP
{
  Magnum::Vector3 position;
};

struct VertexPN
{
  Magnum::Vector3 position;
  Magnum::Vector3 normal;
};

struct VertexPC
{
  Magnum::Vector3 position;
  Magnum::Color4 colour;
};

struct VertexPNC
{
  Magnum::Vector3 position;
  Magnum::Vector3 normal;
  Magnum::Color4 colour;
};

template <typename T>
using Array = Corrade::Containers::Array<T>;
template <typename T>
using ArrayView = Corrade::Containers::ArrayView<T>;

template <typename V>
struct VertexMapper
{
  [[nodiscard]] bool validate(const DataBuffer &src_vertices, const DataBuffer &src_normals,
                              const DataBuffer &src_colours, const ConvertOptions &options) const
  {
    (void)src_vertices;
    (void)src_normals;
    (void)src_colours;
    (void)options;
    return false;
  }
  void operator()(V &vertex, size_t src_index, const DataBuffer &src_vertices,
                  const DataBuffer &src_normals, const DataBuffer &src_colours,
                  const ConvertOptions &options) = delete;

  Array<Magnum::Trade::MeshAttributeData> attributes(const Array<V> &vertices) const = delete;
};


template <>
struct VertexMapper<VertexP>
{
  [[nodiscard]] static bool validate(const DataBuffer &src_vertices, const DataBuffer &src_normals,
                                     const DataBuffer &src_colours, const ConvertOptions &options)
  {
    (void)src_normals;
    (void)src_colours;
    (void)options;
    return src_vertices.isValid();
  }
  Magnum::Vector3 operator()(VertexP &vertex, size_t src_index, const DataBuffer &src_vertices,
                             const DataBuffer &src_normals, const DataBuffer &src_colours,
                             const ConvertOptions &options)
  {
    (void)src_normals;
    (void)src_colours;
    (void)options;
    const auto x = src_vertices.get<Magnum::Float>(src_index, 0);
    const auto y = src_vertices.get<Magnum::Float>(src_index, 1);
    const auto z = src_vertices.get<Magnum::Float>(src_index, 2);
    vertex.position = Magnum::Vector3(x, y, z);
    return vertex.position;
  }

  [[nodiscard]] static Array<Magnum::Trade::MeshAttributeData> attributes(
    const Array<VertexP> &vertices)
  {
    return Array<Magnum::Trade::MeshAttributeData>{
      Corrade::Containers::InPlaceInit,
      { Magnum::Trade::MeshAttributeData{
        Magnum::Trade::MeshAttribute::Position,
        Corrade::Containers::stridedArrayView(vertices, &vertices[0].position, vertices.size(),
                                              sizeof(VertexP)) } }
    };
  }
};


template <>
struct VertexMapper<VertexPN>
{
  [[nodiscard]] static bool validate(const DataBuffer &src_vertices, const DataBuffer &src_normals,
                                     const DataBuffer &src_colours, const ConvertOptions &options)
  {
    (void)src_colours;
    (void)options;
    return src_vertices.isValid() && src_normals.isValid();
  }
  Magnum::Vector3 operator()(VertexPN &vertex, size_t src_index, const DataBuffer &src_vertices,
                             const DataBuffer &src_normals, const DataBuffer &src_colours,
                             const ConvertOptions &options)
  {
    (void)src_colours;
    (void)options;
    const auto x = src_vertices.get<Magnum::Float>(src_index, 0);
    const auto y = src_vertices.get<Magnum::Float>(src_index, 1);
    const auto z = src_vertices.get<Magnum::Float>(src_index, 2);
    vertex.position = Magnum::Vector3(x, y, z);
    const auto nx = src_normals.get<Magnum::Float>(src_index, 0);
    const auto ny = src_normals.get<Magnum::Float>(src_index, 1);
    const auto nz = src_normals.get<Magnum::Float>(src_index, 2);
    vertex.normal = Magnum::Vector3(nx, ny, nz);
    return vertex.position;
  }

  [[nodiscard]] static Array<Magnum::Trade::MeshAttributeData> attributes(
    const Array<VertexPN> &vertices)
  {
    return Array<Magnum::Trade::MeshAttributeData>{
      Corrade::Containers::InPlaceInit,
      {
        Magnum::Trade::MeshAttributeData{
          Magnum::Trade::MeshAttribute::Position,
          Corrade::Containers::stridedArrayView(vertices, &vertices[0].position, vertices.size(),
                                                sizeof(VertexPN)) },
        Magnum::Trade::MeshAttributeData{
          Magnum::Trade::MeshAttribute::Normal,
          Corrade::Containers::stridedArrayView(vertices, &vertices[0].normal, vertices.size(),
                                                sizeof(VertexPN)) },
      }
    };
  }
};


template <>
struct VertexMapper<VertexPC>
{
  [[nodiscard]] static bool validate(const DataBuffer &src_vertices, const DataBuffer &src_normals,
                                     const DataBuffer &src_colours, const ConvertOptions &options)
  {
    (void)src_normals;
    return src_vertices.isValid() && (options.auto_colour || src_colours.isValid());
  }
  Magnum::Vector3 operator()(VertexPC &vertex, size_t src_index, const DataBuffer &src_vertices,
                             const DataBuffer &src_normals, const DataBuffer &src_colours,
                             const ConvertOptions &options)
  {
    (void)src_normals;
    const auto x = src_vertices.get<Magnum::Float>(src_index, 0);
    const auto y = src_vertices.get<Magnum::Float>(src_index, 1);
    const auto z = src_vertices.get<Magnum::Float>(src_index, 2);
    vertex.position = Magnum::Vector3(x, y, z);
    const auto c =
      (src_colours.count()) ? Colour(src_colours.get<uint32_t>(src_index)) : options.default_colour;
    vertex.colour = tes::view::convert(c);
    return vertex.position;
  }

  [[nodiscard]] static Array<Magnum::Trade::MeshAttributeData> attributes(
    const Array<VertexPC> &vertices)
  {
    return Array<Magnum::Trade::MeshAttributeData>{
      Corrade::Containers::InPlaceInit,
      {
        Magnum::Trade::MeshAttributeData{
          Magnum::Trade::MeshAttribute::Position,
          Corrade::Containers::stridedArrayView(vertices, &vertices[0].position, vertices.size(),
                                                sizeof(VertexPC)) },
        Magnum::Trade::MeshAttributeData{
          Magnum::Trade::MeshAttribute::Color,
          Corrade::Containers::stridedArrayView(vertices, &vertices[0].colour, vertices.size(),
                                                sizeof(VertexPC)) },
      }
    };
  }
};


template <>
struct VertexMapper<VertexPNC>
{
  [[nodiscard]] static bool validate(const DataBuffer &src_vertices, const DataBuffer &src_normals,
                                     const DataBuffer &src_colours, const ConvertOptions &options)
  {
    return src_vertices.isValid() && src_normals.isValid() &&
           (options.auto_colour || src_colours.isValid());
  }
  Magnum::Vector3 operator()(VertexPNC &vertex, size_t src_index, const DataBuffer &src_vertices,
                             const DataBuffer &src_normals, const DataBuffer &src_colours,
                             const ConvertOptions &options)
  {
    const auto x = src_vertices.get<Magnum::Float>(src_index, 0);
    const auto y = src_vertices.get<Magnum::Float>(src_index, 1);
    const auto z = src_vertices.get<Magnum::Float>(src_index, 2);
    vertex.position = Magnum::Vector3(x, y, z);
    const auto nx = src_normals.get<Magnum::Float>(src_index, 0);
    const auto ny = src_normals.get<Magnum::Float>(src_index, 1);
    const auto nz = src_normals.get<Magnum::Float>(src_index, 2);
    vertex.normal = Magnum::Vector3(nx, ny, nz);
    const auto c =
      (src_colours.count()) ? Colour(src_colours.get<uint32_t>(src_index)) : options.default_colour;
    vertex.colour = tes::view::convert(c);
    return vertex.position;
  }

  [[nodiscard]] static Array<Magnum::Trade::MeshAttributeData> attributes(
    const Array<VertexPNC> &vertices)
  {
    return Array<Magnum::Trade::MeshAttributeData>{
      Corrade::Containers::InPlaceInit,
      {
        Magnum::Trade::MeshAttributeData{
          Magnum::Trade::MeshAttribute::Position,
          Corrade::Containers::stridedArrayView(vertices, &vertices[0].position, vertices.size(),
                                                sizeof(VertexPNC)) },
        Magnum::Trade::MeshAttributeData{
          Magnum::Trade::MeshAttribute::Normal,
          Corrade::Containers::stridedArrayView(vertices, &vertices[0].normal, vertices.size(),
                                                sizeof(VertexPNC)) },
        Magnum::Trade::MeshAttributeData{
          Magnum::Trade::MeshAttribute::Color,
          Corrade::Containers::stridedArrayView(vertices, &vertices[0].colour, vertices.size(),
                                                sizeof(VertexPNC)) },
      }
    };
  }
};


template <typename V>
Magnum::GL::Mesh convert(const tes::MeshResource &mesh_resource, Magnum::MeshPrimitive draw_type,
                         tes::Bounds<Magnum::Float> &bounds, const ConvertOptions &options)
{
  Array<V> vertices(Corrade::Containers::DefaultInit, mesh_resource.vertexCount());
  Array<Magnum::UnsignedInt> indices(Corrade::Containers::DefaultInit, mesh_resource.indexCount());

  const DataBuffer src_vertices = mesh_resource.vertices();
  const DataBuffer src_normals = mesh_resource.normals();
  const DataBuffer src_colour = mesh_resource.colours();

  VertexMapper<V> mapper;
  if (!mapper.validate(src_vertices, src_normals, src_colour, options))
  {
    return Magnum::GL::Mesh();
  }

  for (size_t i = 0; i < vertices.size(); ++i)
  {
    const auto vertex = mapper(vertices[i], i, src_vertices, src_normals, src_colour, options);
    if (i != 0)
    {
      bounds.expand(tes::Vector3<Magnum::Float>(vertex.x(), vertex.y(), vertex.z()));
    }
    else
    {
      bounds =
        Bounds<Magnum::Float>(tes::Vector3<Magnum::Float>(vertex.x(), vertex.y(), vertex.z()));
    }
  }

  const DataBuffer src_indices = mesh_resource.indices();
  if (src_indices.count())
  {
    for (size_t i = 0; i < indices.size(); ++i)
    {
      indices[i] = src_indices.get<unsigned>(i);
    }
  }
  else if (options.auto_index)
  {
    indices =
      Array<Magnum::UnsignedInt>(Corrade::Containers::DefaultInit, mesh_resource.indexCount());
    for (unsigned i = 0; i < unsigned(indices.size()); ++i)
    {
      indices[i] = i;
    }
  }

  if (!indices.empty())
  {
    const Magnum::Trade::MeshData md(
      draw_type, Magnum::Trade::DataFlags{}, ArrayView<const void>(indices),
      Magnum::Trade::MeshIndexData{ indices }, Magnum::Trade::DataFlags{},
      ArrayView<const void>(vertices), mapper.attributes(vertices));
    return Magnum::MeshTools::compile(md);
  }
  const Magnum::Trade::MeshData md(draw_type, Magnum::Trade::DataFlags{},
                                   ArrayView<const void>(vertices), mapper.attributes(vertices));
  return Magnum::MeshTools::compile(md);
}

Magnum::GL::Mesh convert(const tes::MeshResource &mesh_resource, tes::Bounds<Magnum::Float> &bounds,
                         const ConvertOptions &options)
{
  Magnum::MeshPrimitive primitive = {};

  switch (mesh_resource.drawType())
  {
  case DrawType::Points:
    primitive = Magnum::MeshPrimitive::Points;
    break;
  case DrawType::Lines:
    primitive = Magnum::MeshPrimitive::Lines;
    break;
  case DrawType::Triangles:
    primitive = Magnum::MeshPrimitive::Triangles;
    break;
  case DrawType::Voxels:
    // Requires the right geometry shader to work with this primitive type.
    primitive = Magnum::MeshPrimitive::Points;
    break;
  }

  if (mesh_resource.normals().isValid())
  {
    if (mesh_resource.colours().isValid())
    {
      return convert<VertexPNC>(mesh_resource, primitive, bounds, options);
    }
    return convert<VertexPN>(mesh_resource, primitive, bounds, options);
  }
  if (mesh_resource.colours().isValid() || options.auto_colour)
  {
    return convert<VertexPC>(mesh_resource, primitive, bounds, options);
  }
  return convert<VertexP>(mesh_resource, primitive, bounds, options);
}
}  // namespace tes::view::mesh
