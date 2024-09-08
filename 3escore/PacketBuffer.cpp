//
// author: Kazys Stepanas
//
#include "PacketBuffer.h"

#include "CoreUtil.h"
#include "PacketHeader.h"
#include "PacketReader.h"

#include <algorithm>
#include <cstring>
#include <iostream>
#include <iterator>

namespace tes
{
namespace
{
struct MarkerBytes
{
  std::array<uint8_t, sizeof(kPacketMarker)> bytes;

  MarkerBytes()
  {
    std::memcpy(bytes.data(), &kPacketMarker, sizeof(kPacketMarker));
    networkEndianSwap(bytes);
  }

  [[nodiscard]] uint8_t operator[](size_t idx) const { return bytes[idx]; }
  [[nodiscard]] size_t size() const { return bytes.size(); }
};

int packetMarkerPosition(const uint8_t *bytes, size_t byte_count)
{
  thread_local const MarkerBytes packet_marker;

  for (size_t i = 0; i < byte_count; i += 4)
  {
    if (bytes[i] == packet_marker[0])
    {
      // First marker byte found. Check for the rest.
      bool found = true;
      for (unsigned j = 1; j < packet_marker.size(); ++j)
      {
        found = found && bytes[i + j] == packet_marker[j];
      }

      if (found)
      {
        return static_cast<int>(i);
      }
    }
  }

  return -1;
}
}  // namespace

PacketBuffer::PacketBuffer(size_t capacity)
{
  _packet_buffer.reserve(capacity);
}


PacketBuffer::~PacketBuffer() = default;


int PacketBuffer::addBytes(const uint8_t *bytes, size_t byte_count)
{
  if (_marker_found)
  {
    // Append to the existing buffer.
    appendData(bytes, bytes + byte_count);
    // All bytes accepted.
    return 0;
  }

  // Buffer does not hold any interesting data. Find the marker position in incoming data and
  // initialise the _packet_buffer from there.
  const int marker_pos = packetMarkerPosition(bytes, byte_count);
  if (marker_pos >= 0)
  {
    _marker_found = true;
    appendData(bytes + marker_pos, bytes + byte_count - static_cast<size_t>(marker_pos));
    return marker_pos;
  }

  return -1;
}


PacketHeader *PacketBuffer::extractPacket(std::vector<uint8_t> &buffer)
{
  if (_marker_found && _packet_buffer.size() >= sizeof(PacketHeader))
  {
    // Remember, the CRC appears after the packet payload. We have to include
    // that in our mem copy.
    auto *pending = reinterpret_cast<PacketHeader *>(_packet_buffer.data());
    const PacketReader reader(pending);
    if (reader.packetSize() <= _packet_buffer.size())
    {
      // We have a full packet. Allocate a copy and extract the full packet data.
      const unsigned packet_size = reader.packetSize();
      // FIXME(KS): why copy to a new buffer? Can't we hold a pointer in the buffer, then shift
      // bytes on release?
      buffer.clear();
      std::copy(_packet_buffer.begin(), _packet_buffer.begin() + packet_size,
                std::back_inserter(buffer));

      _marker_found = false;
      if (_packet_buffer.size() > packet_size)
      {
        // Find next marker beyond the packet just returned.
        const int next_marker_pos = packetMarkerPosition(_packet_buffer.data() + packet_size,
                                                         _packet_buffer.size() - packet_size);
        // TODO(KS): should highlight an error when next_marker_pos != packet_size as that suggests
        // some sort of data issue.
        if (next_marker_pos >= 0)
        {
          removeData(int_cast<size_t>(packet_size + static_cast<size_t>(next_marker_pos)));
          _marker_found = true;
        }
        else
        {
          // No new marker. Remove all data.
          removeData(_packet_buffer.size());
        }
      }
      else
      {
        removeData(packet_size);
      }

      return reinterpret_cast<PacketHeader *>(buffer.data());
    }
  }

  return nullptr;
}


template <typename Iter>
void PacketBuffer::appendData(const Iter &begin, const Iter &end)
{
  std::copy(begin, end, std::back_inserter(_packet_buffer));
}


void PacketBuffer::removeData(size_t remove_byte_count)
{
  if (remove_byte_count < _packet_buffer.size())
  {
    const size_t new_size = _packet_buffer.size() - remove_byte_count;
    std::copy(_packet_buffer.begin() + static_cast<ssize_t>(remove_byte_count),
              _packet_buffer.end(), _packet_buffer.begin());
    _packet_buffer.resize(new_size);
  }
  else
  {
    _packet_buffer.resize(0);
  }
}
}  // namespace tes
