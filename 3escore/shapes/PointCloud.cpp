//
// author: Kazys Stepanas
//
#include "PointCloud.h"

#include <3escore/MeshMessages.h>
#include <3escore/Rotation.h>
#include <3escore/SpinLock.h>

#include <algorithm>
#include <cstring>
#include <mutex>

using namespace tes;

namespace tes
{
struct PointCloudImp
{
  SpinLock lock;
  Vector3f *vertices;
  Vector3f *normals;
  Colour *colours;
  unsigned vertexCount;
  unsigned capacity;
  uint32_t id;
  unsigned references;

  inline PointCloudImp(uint32_t id)
    : vertices(nullptr)
    , normals(nullptr)
    , colours(nullptr)
    , vertexCount(0)
    , capacity(0)
    , id(id)
    , references(1)
  {}


  inline ~PointCloudImp()
  {
    delete[] vertices;
    delete[] normals;
    delete[] colours;
  }


  inline PointCloudImp *clone() const
  {
    PointCloudImp *copy = new PointCloudImp(this->id);
    copy->vertexCount = copy->capacity = vertexCount;
    copy->id = id;

    copy->vertices = (vertices && vertexCount) ? new Vector3f[vertexCount] : nullptr;
    std::copy(vertices, vertices + vertexCount, copy->vertices);

    copy->normals = (normals && vertexCount) ? new Vector3f[vertexCount] : nullptr;
    if (copy->normals)
    {
      std::copy(normals, normals + vertexCount, copy->normals);
    }

    copy->colours = (colours && vertexCount) ? new Colour[vertexCount] : nullptr;
    if (copy->colours)
    {
      std::copy(colours, colours + vertexCount, copy->colours);
    }

    copy->references = 1;
    return copy;
  }
};
}  // namespace tes

PointCloud::PointCloud(const PointCloud &other)
  : _imp(other._imp)
{
  std::unique_lock<SpinLock> guard(_imp->lock);
  ++_imp->references;
}


PointCloud::PointCloud(uint32_t id)
  : _imp(new PointCloudImp(id))
{}


PointCloud::~PointCloud()
{
  std::unique_lock<SpinLock> guard(_imp->lock);
  if (_imp->references == 1)
  {
    // Unlock for delete.
    guard.unlock();
    delete _imp;
  }
  else
  {
    --_imp->references;
  }
}


uint32_t PointCloud::id() const
{
  return _imp->id;
}


PointCloud *PointCloud::clone() const
{
  PointCloud *copy = new PointCloud(*this);
  return copy;
}


Transform PointCloud::transform() const
{
  return Transform::identity(false);
}


uint32_t PointCloud::tint() const
{
  return 0xffffffffu;
}


uint8_t PointCloud::drawType(int stream) const
{
  TES_UNUSED(stream);
  return DtPoints;
}


void PointCloud::reserve(const UIntArg &size)
{
  if (_imp->capacity < size)
  {
    setCapacity(size);
  }
}


void PointCloud::resize(const UIntArg &count)
{
  if (_imp->capacity < count)
  {
    reserve(count);
  }

  _imp->vertexCount = count;
}


void PointCloud::squeeze()
{
  if (_imp->capacity > _imp->vertexCount)
  {
    setCapacity(_imp->vertexCount);
  }
}


unsigned PointCloud::capacity() const
{
  return _imp->capacity;
}


unsigned PointCloud::vertexCount(int stream) const
{
  TES_UNUSED(stream);
  return _imp->vertexCount;
}


DataBuffer PointCloud::vertices(int stream) const
{
  TES_UNUSED(stream);
  return DataBuffer(_imp->vertices, _imp->vertexCount);
}


const Vector3f *PointCloud::vertices() const
{
  return _imp->vertices;
}


unsigned PointCloud::indexCount(int stream) const
{
  TES_UNUSED(stream);
  return 0;
}


DataBuffer PointCloud::indices(int stream) const
{
  TES_UNUSED(stream);
  return DataBuffer();
}


DataBuffer PointCloud::normals(int stream) const
{
  TES_UNUSED(stream);
  return DataBuffer(_imp->normals, _imp->normals ? _imp->vertexCount : 0);
}


const Vector3f *PointCloud::normals() const
{
  return _imp->normals;
}


DataBuffer PointCloud::colours(int stream) const
{
  TES_UNUSED(stream);
  return DataBuffer(_imp->colours, _imp->colours ? _imp->vertexCount : 0);
}


const Colour *PointCloud::colours() const
{
  return _imp->colours;
}


DataBuffer PointCloud::uvs(int) const
{
  return DataBuffer();
}


void PointCloud::addPoints(const Vector3f *points, const UIntArg &count)
{
  if (count)
  {
    copyOnWrite();
    unsigned initial = _imp->vertexCount;
    resize(_imp->vertexCount + count.i);
    std::copy(points, points + count.i, _imp->vertices + initial);

    // Initialise other data
    for (unsigned i = initial; i < _imp->vertexCount; ++i)
    {
      _imp->normals[i] = Vector3f::Zero;
    }

    const Colour c = Colour(Colour::White);
    for (unsigned i = initial; i < _imp->vertexCount; ++i)
    {
      _imp->colours[i] = c;
    }
  }
}


void PointCloud::addPoints(const Vector3f *points, const Vector3f *normals, const UIntArg &count)
{
  if (count)
  {
    copyOnWrite();
    unsigned initial = _imp->vertexCount;
    resize(_imp->vertexCount + count.i);
    std::copy(points, points + count.i, _imp->vertices + initial);
    std::copy(normals, normals + count.i, _imp->normals + initial);

    // Initialise other data
    const Colour c = Colour(Colour::White);
    for (unsigned i = initial; i < _imp->vertexCount; ++i)
    {
      _imp->colours[i] = c;
    }
  }
}


void PointCloud::addPoints(const Vector3f *points, const Vector3f *normals, const Colour *colours,
                           const UIntArg &count)
{
  if (count)
  {
    copyOnWrite();
    unsigned initial = _imp->vertexCount;
    resize(_imp->vertexCount + count.i);
    std::copy(points, points + count.i, _imp->vertices + initial);
    std::copy(normals, normals + count.i, _imp->normals + initial);
    std::copy(colours, colours + count.i, _imp->colours + initial);
  }
}


void PointCloud::setNormal(const UIntArg &index, const Vector3f &normal)
{
  if (index < _imp->vertexCount)
  {
    copyOnWrite();
    _imp->normals[index.i] = normal;
  }
}


void PointCloud::setColour(const UIntArg &index, const Colour &colour)
{
  if (index < _imp->vertexCount)
  {
    copyOnWrite();
    _imp->colours[index.i] = colour;
  }
}


void PointCloud::setPoints(const UIntArg &index, const Vector3f *points, const UIntArg &count)
{
  if (index >= _imp->vertexCount)
  {
    return;
  }

  unsigned limitedCount = count;
  if (index.i + limitedCount > _imp->vertexCount)
  {
    limitedCount = index.i + count.i - _imp->vertexCount;
  }

  if (!limitedCount)
  {
    return;
  }

  copyOnWrite();
  std::copy(points, points + limitedCount, _imp->vertices + index.i);
}


void PointCloud::setPoints(const UIntArg &index, const Vector3f *points, const Vector3f *normals,
                           const UIntArg &count)
{
  if (index >= _imp->vertexCount)
  {
    return;
  }

  unsigned limitedCount = count;
  if (index.i + limitedCount > _imp->vertexCount)
  {
    limitedCount = index.i + count.i - _imp->vertexCount;
  }

  if (!limitedCount)
  {
    return;
  }

  copyOnWrite();
  std::copy(points, points + limitedCount, _imp->vertices + index.i);
  std::copy(normals, normals + limitedCount, _imp->normals + index.i);
}


void PointCloud::setPoints(const UIntArg &index, const Vector3f *points, const Vector3f *normals,
                           const Colour *colours, const UIntArg &count)
{
  if (index >= _imp->vertexCount)
  {
    return;
  }

  unsigned limitedCount = count;
  if (index.i + limitedCount > _imp->vertexCount)
  {
    limitedCount = index.i + count.i - _imp->vertexCount;
  }

  if (!limitedCount)
  {
    return;
  }

  copyOnWrite();
  std::copy(points, points + limitedCount, _imp->vertices + index.i);
  std::copy(normals, normals + limitedCount, _imp->normals + index.i);
  std::copy(colours, colours + limitedCount, _imp->colours + index.i);
}


void PointCloud::setCapacity(unsigned size)
{
  if (_imp->capacity == size)
  {
    // Already at the requested size.
    return;
  }

  copyOnWrite();
  // Check capacity again. The copyOnWrite() may have set them to be the same.
  if (_imp->capacity != size)
  {
    if (!size)
    {
      delete[] _imp->vertices;
      delete[] _imp->normals;
      delete[] _imp->colours;
      _imp->vertices = _imp->normals = nullptr;
      _imp->colours = nullptr;
      _imp->capacity = 0;
      _imp->vertexCount = 0;
      return;
    }

    Vector3f *points = new Vector3f[size];
    Vector3f *normals = new Vector3f[size];
    Colour *colours = new Colour[size];

    unsigned vertexCount = std::min(_imp->vertexCount, size);
    if (_imp->capacity)
    {
      // Copy existing data.
      if (vertexCount)
      {
        std::copy(_imp->vertices, _imp->vertices + vertexCount, points);
        std::copy(_imp->normals, _imp->normals + vertexCount, normals);
        std::copy(_imp->colours, _imp->colours + vertexCount, colours);
      }

      delete[] _imp->vertices;
      delete[] _imp->normals;
      delete[] _imp->colours;
    }

    _imp->vertices = points;
    _imp->normals = normals;
    _imp->colours = colours;
    _imp->capacity = size;
    _imp->vertexCount = vertexCount;
  }
}


void PointCloud::copyOnWrite()
{
  std::unique_lock<SpinLock> guard(_imp->lock);
  if (_imp->references > 1)
  {
    --_imp->references;
    _imp = _imp->clone();
  }
}


bool PointCloud::processCreate(const MeshCreateMessage &msg,
                               const ObjectAttributes<double> &attributes)
{
  if (msg.draw_type != DtPoints)
  {
    return false;
  }

  copyOnWrite();
  _imp->id = msg.mesh_id;

  _imp->vertexCount = msg.vertex_count;
  delete _imp->vertices;
  delete _imp->normals;
  delete _imp->colours;
  _imp->capacity = msg.vertex_count;
  _imp->vertices = new Vector3f[msg.vertex_count];
  _imp->normals = nullptr;  // Pending.
  _imp->colours = nullptr;  // Pending

  Transform transform(Vector3d(attributes.position), Quaterniond(attributes.rotation),
                      Vector3d(attributes.scale), msg.flags & McfDoublePrecision);

  // Does not accept a transform.
  if (!transform.isEqual(Transform::identity()))
  {
    return false;
  }

  // Does not accept a tint.
  if (attributes.colour != 0xffffffffu)
  {
    return false;
  }

  return true;
}


bool PointCloud::processVertices(const MeshComponentMessage &msg, unsigned offset,
                                 const DataBuffer &stream)
{
  TES_UNUSED(msg);
  static_assert(sizeof(Vector3f) == sizeof(float) * 3, "Vertex size mismatch");
  copyOnWrite();
  unsigned wrote = 0;

  for (unsigned i = 0; i + offset < _imp->vertexCount && i < stream.count(); ++i)
  {
    for (int j = 0; j < 3; ++j)
    {
      _imp->vertices[i + offset][j] = stream.get<float>(i, j);
    }
    ++wrote;
  }

  return wrote == stream.count();
}


bool PointCloud::processColours(const MeshComponentMessage &msg, unsigned offset,
                                const DataBuffer &stream)
{
  TES_UNUSED(msg);
  copyOnWrite();
  unsigned wrote = 0;
  if (_imp->colours == nullptr)
  {
    _imp->colours = new Colour[_imp->vertexCount];
  }

  for (unsigned i = 0; i + offset < _imp->vertexCount && i < stream.count(); ++i)
  {
    _imp->colours[i + offset] = Colour(stream.get<uint8_t>(i, 0), stream.get<uint8_t>(i, 1),
                                       stream.get<uint8_t>(i, 2), stream.get<uint8_t>(i, 3));
    ++wrote;
  }

  return wrote == stream.count();
}


bool PointCloud::processNormals(const MeshComponentMessage &msg, unsigned offset,
                                const DataBuffer &stream)
{
  TES_UNUSED(msg);
  static_assert(sizeof(Vector3f) == sizeof(float) * 3, "Normal size mismatch");

  copyOnWrite();
  unsigned wrote = 0;
  if (_imp->normals == nullptr)
  {
    _imp->normals = new Vector3f[_imp->vertexCount];
  }

  for (unsigned i = 0; i + offset < _imp->vertexCount && i < stream.count(); ++i)
  {
    for (int j = 0; j < 3; ++j)
    {
      _imp->normals[i + offset][j] = stream.get<float>(i, j);
    }
    ++wrote;
  }

  return wrote == stream.count();
}
