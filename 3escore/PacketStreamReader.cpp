//
// author: Kazys Stepanas
//
#include "PacketStreamReader.h"

#include "CoreUtil.h"
#include "PacketReader.h"

#include <cstddef>
#include <cstring>

namespace tes
{
PacketStreamReader::PacketStreamReader()
{
  const auto packet_marker = networkEndianSwapValue(tes::kPacketMarker);
  std::memcpy(_marker_bytes.data(), &packet_marker, sizeof(packet_marker));
  _buffer.reserve(_chunk_size);
}

PacketStreamReader::PacketStreamReader(std::shared_ptr<std::istream> stream)
  : PacketStreamReader()
{
  // An initialiser delegate must stand alone, so member initialisation must appear here.
  // NOLINTNEXTLINE(cppcoreguidelines-prefer-member-initializer)
  _stream = std::move(stream);
  if (_stream)
  {
    _current_packet_pos = _stream->tellg();
  }
}


PacketStreamReader::~PacketStreamReader() = default;


void PacketStreamReader::setStream(std::shared_ptr<std::istream> stream)
{
  std::swap(stream, _stream);
  _buffer.clear();
  if (_stream)
  {
    _current_packet_pos = _stream->tellg();
  }
}


PacketStreamReader::ExtractedPacket PacketStreamReader::extractPacket()
{
  if (!_stream)
  {
    return { nullptr, Status::NoStream, 0 };
  }

  consume();

  // Read a chunk from the stream.
  if (_buffer.empty())
  {
    if (readMore(_chunk_size - _buffer.size()) == 0)
    {
      // We have no data to read more.
      return { nullptr, Status::End, 0 };
    }
  }

  // Scan for the buffer start.
  for (size_t i = 0; i < _buffer.size(); ++i)
  {
    Status status = Status::Success;
    if (checkMarker(_buffer, i))
    {
      // Marker found. Shift down to comsume trash at the start of the buffer.
      if (i > 0)
      {
        std::copy(_buffer.begin() + static_cast<ssize_t>(i), _buffer.end(), _buffer.begin());
        _buffer.resize(_buffer.size() - i);
        status = Status::Dropped;
      }

      if (_buffer.size() < sizeof(PacketHeader))
      {
        readMore(sizeof(PacketHeader) - _buffer.size());
        if (_buffer.size() < sizeof(PacketHeader))
        {
          // Can't read sufficient bytes. Abort.
          return { nullptr, Status::Incomplete, 0 };
        }
      }

      // Check the packet size and work out how much more to read.
      const auto target_size = calcExpectedSize();
      // Read the full payload.
      if (_buffer.size() < target_size)
      {
        readMore(target_size - _buffer.size());
        if (_buffer.size() < target_size)
        {
          // Failed to read enough.
          if (_stream->eof())
          {
            // Failed to read enough and we've reached the end of the stream.
            // We are done.
            _buffer.clear();
          }
          return { nullptr, Status::Incomplete, 0 };
        }
      }

      // We have our packet.
      // Mark to consume on next call.
      // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
      return { reinterpret_cast<const PacketHeader *>(_buffer.data()), status,
               _current_packet_pos };
    }
  }

  return { nullptr, Status::Unavailable, 0 };
}


void PacketStreamReader::seek(std::istream::pos_type position)
{
  _buffer.clear();
  if (_stream)
  {
    _stream->clear();
    _stream->seekg(position);
  }
}


size_t PacketStreamReader::readMore(size_t more_count)
{
  static_assert(sizeof(*_buffer.data()) == sizeof(char));
  if (isEof())
  {
    return 0;
  }

  auto have_count = _buffer.size();
  _buffer.resize(have_count + more_count);
  // Note(KS): I was using readsome() because that returns the count read, but it was also not
  // working as expected.
  // NOLINTNEXTLINE(cppcoreguidelines-pro-*)
  _stream->read(reinterpret_cast<char *>(_buffer.data()) + have_count,
                int_cast<unsigned>(more_count));
  const auto read_count = _stream->gcount();
  if (read_count == std::numeric_limits<std::streamsize>::max())
  {
    return 0;
  }
  _buffer.resize(have_count + read_count);
  return read_count;
}


bool PacketStreamReader::checkMarker(const std::vector<uint8_t> &buffer, size_t i)
{
  for (size_t j = 0; j < _marker_bytes.size() && i + j < buffer.size(); ++j)
  {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
    if (_buffer[i + j] != _marker_bytes[j])
    {
      return false;
    }
  }
  return true;
}


void PacketStreamReader::consume()
{
  if (_buffer.size() < sizeof(PacketHeader))
  {
    // Not possible. Too small.
    return;
  }

  if (!checkMarker(_buffer, 0))
  {
    // Not at a valid packet.
    return;
  }

  const auto target_size = calcExpectedSize();
  if (_buffer.size() >= target_size)
  {
    // Update cursor.
    _current_packet_pos += target_size;
    // Consume.
    std::copy(_buffer.begin() + static_cast<ssize_t>(target_size), _buffer.end(), _buffer.begin());
    _buffer.resize(_buffer.size() - target_size);
  }
}


size_t PacketStreamReader::calcExpectedSize()
{
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
  const auto *header = reinterpret_cast<const PacketHeader *>(_buffer.data());
  auto payload_size = networkEndianSwapValue(header->payload_size);
  if ((networkEndianSwapValue(header->flags) & PFNoCrc) == 0)
  {
    payload_size += sizeof(PacketReader::CrcType);
  }
  return sizeof(PacketHeader) + payload_size;
}
}  // namespace tes
