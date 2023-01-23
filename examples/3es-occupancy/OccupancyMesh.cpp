//
// author Kazys Stepanas
//
#include "OccupancyMesh.h"

#include "p2p.h"

#ifdef TES_ENABLE
#include <3escore/Colour.h>
#include <3escore/ServerMacros.h>
#include <3escore/TransferProgress.h>

using namespace tes;

struct OccupancyMeshDetail
{
  std::vector<tes::Vector3f> vertices;
  // Define the render extents for the voxels.
  std::vector<tes::Vector3f> normals;
  std::vector<uint32_t> colours;
  // std::vector<uint32_t> indices;
  /// Tracks indices of unused vertices in the vertex array.
  std::vector<uint32_t> unusedVertexList;
  /// Maps voxel keys to their vertex indices.
  KeyToIndexMap voxelIndexMap;
};

namespace
{
// bool validateVertex(const Vector3f &v)
// {
//   for (int i = 0; i < 3; ++i)
//   {
//     if (std::abs(v[i]) > 1e6f)
//     {
//       return false;
//     }
//   }
//   return true;
// }


uint32_t nodeColour(const octomap::OcTree::NodeType *node, const octomap::OcTree &map)
{
  const float intensity = float((node->getOccupancy() - map.getOccupancyThres()) / (1.0 - map.getOccupancyThres()));
  // const float intensity = (node) ? float(node->getOccupancy()) : 0;
  const int c = int(255 * intensity);
  return tes::Colour(c, c, c).c;
}
}  // namespace

OccupancyMesh::OccupancyMesh(unsigned meshId, octomap::OcTree &map)
  : _map(map)
  , _id(meshId)
  , _detail(new OccupancyMeshDetail)
{
  // Expose the mesh resource.
  g_tesServer->referenceResource(this);
}


OccupancyMesh::~OccupancyMesh()
{
  g_tesServer->releaseResource(this);
  delete _detail;
}

uint32_t OccupancyMesh::id() const
{
  return _id;
}


tes::Transform OccupancyMesh::transform() const
{
  return tes::Transform::identity(false);
}


uint32_t OccupancyMesh::tint() const
{
  return 0xFFFFFFFFu;
}


uint8_t OccupancyMesh::drawType(int stream) const
{
  TES_UNUSED(stream);
  return tes::DtVoxels;
}


unsigned OccupancyMesh::vertexCount(int stream) const
{
  TES_UNUSED(stream);
  return (unsigned)_detail->vertices.size();
}


unsigned OccupancyMesh::indexCount(int stream) const
{
  TES_UNUSED(stream);
  // return (unsigned)_detail->indices.size();
  return 0;
}


DataBuffer OccupancyMesh::vertices(int stream) const
{
  TES_UNUSED(stream);
  return DataBuffer(_detail->vertices);
}


DataBuffer OccupancyMesh::indices(int stream) const
{
  TES_UNUSED(stream);
  return DataBuffer();
}

DataBuffer OccupancyMesh::normals(int stream) const
{
  TES_UNUSED(stream);
  return DataBuffer(_detail->normals);
}


DataBuffer OccupancyMesh::uvs(int stream) const
{
  TES_UNUSED(stream);
  return DataBuffer();
}


DataBuffer OccupancyMesh::colours(int stream) const
{
  TES_UNUSED(stream);
  return DataBuffer(_detail->colours);
}

tes::Resource *OccupancyMesh::clone() const
{
  OccupancyMesh *copy = new OccupancyMesh(_id, _map);
  *copy->_detail = *_detail;
  return copy;
}


int OccupancyMesh::transfer(tes::PacketWriter &packet, unsigned byteLimit, tes::TransferProgress &progress) const
{
  // Build the voxel set if required.
  if (_detail->voxelIndexMap.empty())
  {
    _detail->vertices.clear();
    _detail->colours.clear();
    for (auto node = _map.begin_leafs(); node != _map.end(); ++node)
    {
      if (_map.isNodeOccupied(*node))
      {
        // Add voxel.
        _detail->voxelIndexMap.insert(std::make_pair(node.getKey(), uint32_t(_detail->vertices.size())));
        _detail->vertices.push_back(p2p(_map.keyToCoord(node.getKey())));
        // Normals represent voxel half extents.
        _detail->normals.push_back(Vector3f(float(0.5f * _map.getResolution())));
        _detail->colours.push_back(nodeColour(&*node, _map));
      }
    }
  }

  return tes::MeshResource::transfer(packet, byteLimit, progress);
}


void OccupancyMesh::update(const UnorderedKeySet &newlyOccupied, const UnorderedKeySet &newlyFree,
                           const UnorderedKeySet &touchedOccupied)
{
  if (newlyOccupied.empty() && newlyFree.empty() && touchedOccupied.empty())
  {
    // Nothing to do.
    return;
  }

  if (g_tesServer->connectionCount() == 0)
  {
    // No-one to send to.
    _detail->vertices.clear();
    _detail->normals.clear();
    _detail->colours.clear();
    //_detail->indices.clear();
    _detail->unusedVertexList.clear();
    _detail->voxelIndexMap.clear();
    return;
  }

  // Start by removing freed nodes.
  size_t initialUnusedVertexCount = _detail->unusedVertexList.size();
  std::vector<uint32_t> modifiedVertices;
  for (const octomap::OcTreeKey &key : newlyFree)
  {
    // Resolve the index for this voxel.
    auto voxelLookup = _detail->voxelIndexMap.find(key);
    if (voxelLookup != _detail->voxelIndexMap.end())
    {
      // Invalidate the voxel.
      _detail->colours[voxelLookup->second] = 0u;
      _detail->unusedVertexList.push_back(voxelLookup->second);
      modifiedVertices.push_back(voxelLookup->second);
      _detail->voxelIndexMap.erase(voxelLookup);
    }
  }

  // Now added occupied nodes, initially from the free list.
  size_t processedOccupiedCount = 0;
  auto occupiedIter = newlyOccupied.begin();
  while (!_detail->unusedVertexList.empty() && occupiedIter != newlyOccupied.end())
  {
    const uint32_t vertexIndex = _detail->unusedVertexList.back();
    const octomap::OcTreeKey key = *occupiedIter;
    const octomap::OcTree::NodeType *node = _map.search(key);
    const bool markAsModified = _detail->unusedVertexList.size() <= initialUnusedVertexCount;
    _detail->unusedVertexList.pop_back();
    ++occupiedIter;
    ++processedOccupiedCount;
    _detail->vertices[vertexIndex] = p2p(_map.keyToCoord(key));
    // validateVertex(_detail->vertices[vertexIndex]);
    _detail->colours[vertexIndex] = nodeColour(node, _map);
    _detail->voxelIndexMap.insert(std::make_pair(key, vertexIndex));
    // Only mark as modified if this vertex wasn't just invalidate by removal.
    // It will already be on the list otherwise.
    if (markAsModified)
    {
      modifiedVertices.push_back(vertexIndex);
    }
  }

  // Send messages for individually changed voxels.
  // Start a mesh redefinition message.
  std::vector<uint8_t> buffer(0xffffu);
  tes::PacketWriter packet(buffer.data(), (uint16_t)buffer.size());
  tes::MeshRedefineMessage msg;
  tes::MeshComponentMessage cmpmsg;
  tes::MeshFinaliseMessage finalmsg;
  tes::ObjectAttributesd attributes;

  // Work out how many vertices we'll have after all modifications are done.
  size_t oldVertexCount = _detail->vertices.size();
  size_t newVertexCount = _detail->vertices.size();
  if (newlyOccupied.size() - processedOccupiedCount > _detail->unusedVertexList.size())
  {
    // We have more occupied vertices than available in the free list.
    // This means we will add new vertices.
    newVertexCount += newlyOccupied.size() - processedOccupiedCount - _detail->unusedVertexList.size();
  }

  msg.meshId = _id;
  msg.vertexCount = (uint32_t)newVertexCount;
  msg.indexCount = 0;
  msg.drawType = drawType(0);
  attributes.identity();

  packet.reset(tes::MtMesh, tes::MeshRedefineMessage::MessageId);
  msg.write(packet, attributes);

  packet.finalise();
  g_tesServer->send(packet);

  // Next update changed triangles.
  cmpmsg.meshId = id();
  cmpmsg.reserved = 0;
  cmpmsg.count = 1;

  // Update modified vertices, one at a time.
  for (uint32_t vertexIndex : modifiedVertices)
  {
    cmpmsg.offset = vertexIndex;
    packet.reset(tes::MtMesh, tes::MmtVertex);
    cmpmsg.elementType = McetFloat32;
    cmpmsg.write(packet);
    // Write the invalid value.
    packet.writeArray<float>(_detail->vertices[vertexIndex].v, 3);
    packet.finalise();
    g_tesServer->send(packet);

    // Send colour and position update.
    packet.reset(tes::MtMesh, tes::MmtVertexColour);
    cmpmsg.elementType = McetUInt32;
    cmpmsg.write(packet);
    // Write the invalid value.
    packet.writeArray<uint32_t>(&_detail->colours[vertexIndex], 1);
    packet.finalise();
    g_tesServer->send(packet);
  }

  // Add remaining vertices and send a bulk modification message.
  for (; occupiedIter != newlyOccupied.end(); ++occupiedIter, ++processedOccupiedCount)
  {
    const uint32_t vertexIndex = uint32_t(_detail->vertices.size());
    const octomap::OcTreeKey key = *occupiedIter;
    _detail->voxelIndexMap.insert(std::make_pair(key, vertexIndex));
    //_detail->indices.push_back(uint32_t(_detail->vertices.size()));
    _detail->vertices.push_back(p2p(_map.keyToCoord(key)));
    // validateVertex(_detail->vertices.back());
    // Normals represent voxel half extents.
    _detail->normals.push_back(Vector3f(float(0.5f * _map.getResolution())));
    _detail->colours.push_back(0xffffffffu);
  }

  // Send bulk messages for new vertices.
  if (oldVertexCount != newVertexCount)
  {
    const uint16_t transferLimit = 5001;
    // Send colour and position update.
    cmpmsg.offset = uint32_t(oldVertexCount);

    while (cmpmsg.offset < newVertexCount)
    {
      cmpmsg.count = uint16_t(std::min<size_t>(transferLimit, newVertexCount - cmpmsg.offset));

      cmpmsg.elementType = McetFloat32;
      packet.reset(tes::MtMesh, tes::MmtVertex);
      cmpmsg.write(packet);
      packet.writeArray<float>(_detail->vertices[cmpmsg.offset].v, cmpmsg.count * 3);
      packet.finalise();
      g_tesServer->send(packet);

      cmpmsg.elementType = McetFloat32;
      packet.reset(tes::MtMesh, tes::MmtNormal);
      cmpmsg.write(packet);
      packet.writeArray<float>(_detail->normals[cmpmsg.offset].v, cmpmsg.count * 3);
      packet.finalise();
      g_tesServer->send(packet);

      cmpmsg.elementType = McetUInt32;
      packet.reset(tes::MtMesh, tes::MmtVertexColour);
      cmpmsg.write(packet);
      packet.writeArray<uint32_t>(&_detail->colours[cmpmsg.offset], cmpmsg.count);
      packet.finalise();
      g_tesServer->send(packet);

      // Calculate next batch.
      cmpmsg.offset += cmpmsg.count;
    }
  }

  // Update colours for touched occupied
  if (!touchedOccupied.empty())
  {
    for (auto key : touchedOccupied)
    {
      const octomap::OcTree::NodeType *node = _map.search(key);
      auto indexSearch = _detail->voxelIndexMap.find(key);
      if (node && indexSearch != _detail->voxelIndexMap.end())
      {
        const unsigned voxelIndex = indexSearch->second;
        _detail->colours[voxelIndex] = nodeColour(node, _map);

        packet.reset(tes::MtMesh, tes::MmtVertexColour);
        cmpmsg.offset = voxelIndex;
        cmpmsg.count = 1;
        cmpmsg.elementType = McetUInt32;
        cmpmsg.write(packet);
        packet.writeArray<uint32_t>(&_detail->colours[voxelIndex], 1);
        packet.finalise();
        g_tesServer->send(packet);
      }
    }
  }

  // Finalise the modifications.
  finalmsg.meshId = _id;
  // Rely on EDL shader.
  finalmsg.flags = 0;  // tes::MffCalculateNormals;
  packet.reset(tes::MtMesh, finalmsg.MessageId);
  finalmsg.write(packet);
  packet.finalise();
  g_tesServer->send(packet);
}

#endif  // TES_ENABLE
