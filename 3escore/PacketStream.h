//
// author: Kazys Stepanas
//
#ifndef TES_CORE_HEADER_STREAM_H
#define TES_CORE_HEADER_STREAM_H

#include "CoreConfig.h"

#include "Endian.h"
#include "Enum.h"
#include "PacketHeader.h"

#include <limits>
#include <utility>

namespace tes
{
/// Defies the packet CRC type.
using CrcType = uint16_t;

/// Defines the upper limit for the size of any one 3es packet.
///
/// This is calculated as the size of the packet plus the payload (determined by
/// @c PacketHeader::payload_size ) plus the @c CrcType size.
constexpr size_t kMaxPacketSize = std::numeric_limits<decltype(PacketHeader::payload_size)>::max() +
                                  sizeof(PacketHeader) + sizeof(CrcType);

// Casting and pointer arithmetic are fundamental the the PacketStream.
// clang-format off
// NOLINTBEGIN(cppcoreguidelines-pro-type-reinterpret-cast, cppcoreguidelines-pro-bounds-pointer-arithmetic)
// clang-format on

/// Status bits for @c PacketStream<T>
enum class PacketStatus : uint16_t
{
  /// No issues.
  Zero = 0,
  /// End at of packet/stream.
  EOP = (1u << 0u),
  /// Set after an operation fails.
  Fail = (1u << 1u),
  /// Read only stream?
  ReadOnly = (1u << 2u),
  /// Is the CRC valid?
  CrcValid = (1u << 3u),
};

TES_ENUM_FLAGS(PacketStatus, uint16_t);

/// A utility class used for managing read/write operations to a @c PacketHeader payload.
///
/// The template type is intended to be either a @c PacketReader or a @c const @c PacketHeader
/// for use with @c PacketWriter and @c PacketReader respectively.
template <class Header>
class PacketStream
{
public:
  /// Defies the packet CRC type.
  using CrcType = tes::CrcType;
  using HeaderType = std::remove_const_t<Header>;

  /// Control values for seeking.
  enum class SeekPos
  {
    Begin,    ///< Seek from the beginning of the stream.
    Current,  ///< Seek from the current position.
    End       ///< Seek from the end of the stream.
  };

  /// Create a stream to read from beginning at @p packet.
  /// @param packet The beginning of the data packet.
  PacketStream(Header *packet);

  PacketStream(const PacketStream<Header> &other) = delete;
  PacketStream(PacketStream<Header> &&other) noexcept;

  ~PacketStream() = default;

  PacketStream<Header> &operator=(const PacketStream<Header> &other) = delete;
  PacketStream<Header> &operator=(PacketStream<Header> &&other) noexcept;

  // PacketHeader member access. Ensures network endian swap as required.
  /// Fetch the marker bytes in local endian.
  /// @return The @c PacketHeader::marker bytes.
  [[nodiscard]] uint32_t marker() const { return networkEndianSwapValue(_packet->marker); }
  /// Fetch the major version bytes in local endian.
  /// @return The @c PacketHeader::version_major bytes.
  [[nodiscard]] uint16_t versionMajor() const
  {
    return networkEndianSwapValue(_packet->version_major);
  }
  /// Fetch the minor version bytes in local endian.
  /// @return The @c PacketHeader::version_minor bytes.
  [[nodiscard]] uint16_t versionMinor() const
  {
    return networkEndianSwapValue(_packet->version_minor);
  }
  /// Fetch the payload size bytes in local endian.
  /// @return The @c PacketHeader::payloadSize bytes.
  [[nodiscard]] uint16_t payloadSize() const
  {
    return networkEndianSwapValue(_packet->payload_size);
  }
  /// Returns the size of the packet plus payload, giving the full data packet size including the
  /// CRC.
  /// @return PacketHeader data size (bytes).
  [[nodiscard]] uint16_t packetSize() const
  {
    return static_cast<uint16_t>(sizeof(Header) + payloadSize() +
                                 (((packet().flags & PFNoCrc) == 0) ? sizeof(CrcType) : 0));
  }
  /// Fetch the routing ID bytes in local endian.
  /// @return The @c PacketHeader::routing_id bytes.
  [[nodiscard]] uint16_t routingId() const { return networkEndianSwapValue(_packet->routing_id); }
  /// Fetch the message ID bytes in local endian.
  /// @return The @c PacketHeader::message_id bytes.
  [[nodiscard]] uint16_t messageId() const { return networkEndianSwapValue(_packet->message_id); }
  /// Fetch the flags bytes in local endian.
  /// @return the @c PacketHeader::flags bytes.
  [[nodiscard]] uint8_t flags() const { return networkEndianSwapValue(_packet->flags); }
  /// Fetch the CRC bytes in local endian.
  /// Invalid for packets with the @c PFNoCrc flag set.
  /// @return The packet's CRC value.
  [[nodiscard]] CrcType crc() const { return networkEndianSwapValue(*crcPtr()); }
  /// Fetch a pointer to the CRC bytes.
  /// Invalid for packets with the @c PFNoCrc flag set.
  /// @return A pointer to the CRC location.
  template <typename T = Header, std::enable_if_t<!std::is_const_v<T>, bool> = true>
  [[nodiscard]] CrcType *crcPtr()
  {
    // CRC appears after the payload.
    uint8_t *pos = reinterpret_cast<uint8_t *>(_packet) + sizeof(Header) + payloadSize();
    return reinterpret_cast<CrcType *>(pos);
  }
  /// @overload
  [[nodiscard]] const CrcType *crcPtr() const;

  /// Report the @c Status bits.
  /// @return The @c Status flags.
  [[nodiscard]] PacketStatus status() const;

  /// At end of packet/stream?
  /// @return True if at end of packet.
  [[nodiscard]] bool isEop() const { return (_status & PacketStatus::EOP) != PacketStatus::Zero; }
  /// Status OK?
  /// @return True if OK
  [[nodiscard]] bool isOk() const { return !isFail(); }
  /// Fail bit set?
  /// @return True if fail bit is set.
  [[nodiscard]] bool isFail() const { return (_status & PacketStatus::Fail) != PacketStatus::Zero; }
  /// Read only stream?
  /// @return True if read only.
  [[nodiscard]] bool isReadOnly() const
  {
    return (_status & PacketStatus::ReadOnly) != PacketStatus::Zero;
  }
  /// CRC validated?
  /// @return True if CRC has been validated.
  [[nodiscard]] bool isCrcValid() const
  {
    return (_status & PacketStatus::CrcValid) != PacketStatus::Zero;
  }

  /// Access the head of the packet buffer, for direct @p PacketHeader access.
  /// Note: values are in network Endian.
  /// @return A reference to the @c PacketHeader.
  [[nodiscard]] Header &packet() const;

  /// Tell the current stream position.
  /// @return The current position.
  [[nodiscard]] uint16_t tell() const;
  /// Seek to the indicated position.
  /// @param offset Seek offset from @p pos.
  /// @param pos The seek reference position.
  bool seek(int offset, SeekPos pos = SeekPos::Begin);
  /// Direct payload pointer access.
  /// @return The start of the payload bytes.
  [[nodiscard]] const uint8_t *payload() const;

  /// Swap the contents of this object with that of @p other .
  /// @param other The object to swap with.
  void swap(PacketStream<Header> &other) noexcept
  {
    using std::swap;
    swap(_packet, other._packet);
    swap(_status, other._status);
    swap(_payload_position, other._payload_position);
  }

protected:
  // NOLINTBEGIN(cppcoreguidelines-non-private-member-variables-in-classes)
  Header *_packet = nullptr;                  ///< Packet header and buffer start address.
  PacketStatus _status = PacketStatus::Zero;  ///< @c Status bits.
  uint16_t _payload_position = 0u;            ///< Payload cursor.
  // NOLINTEND(cppcoreguidelines-non-private-member-variables-in-classes)

  /// Type traits: is @c T const?
  template <class T>
  struct IsConst
  {
    /// Check the traits.
    /// @return True if @p T is const.
    [[nodiscard]] bool check() const { return false; }
  };

  /// Type traits: is @c T const?
  template <class T>
  struct IsConst<const T>
  {
    /// Check the traits.
    /// @return True if @p T is const.
    [[nodiscard]] bool check() const { return true; }
  };
};

TES_EXTERN template class TES_CORE_API PacketStream<PacketHeader>;
TES_EXTERN template class TES_CORE_API PacketStream<const PacketHeader>;

template <class Header>
PacketStream<Header>::PacketStream(Header *packet)
  : _packet(packet)
{
  if (IsConst<Header>().check())
  {
    _status |= PacketStatus::ReadOnly;
  }
}


template <class Header>
PacketStream<Header>::PacketStream(PacketStream<Header> &&other) noexcept
  : _packet(std::exchange(other._packet, nullptr))
  , _status(std::exchange(other._status, PacketStatus::Zero))
  , _payload_position(std::exchange(_payload_position, 0u))
{}


// clang-format off
template <class Header>
PacketStream<Header> &PacketStream<Header>::operator=(PacketStream<Header> &&other) noexcept
{
  swap(other);
  return *this;
}


template <class Header>
bool PacketStream<Header>::seek(int offset, SeekPos pos)
{
  switch (pos)
  {
  case SeekPos::Begin:
    if (offset <= payloadSize())
    {
      _payload_position = static_cast<uint16_t>(offset);
      return true;
    }
    break;

  case SeekPos::Current:
    if (offset >= 0 && offset + _payload_position <= payloadSize() ||
        offset < 0 && _payload_position >= -offset)
    {
      _payload_position = static_cast<uint16_t>(_payload_position + offset);
      return true;
    }
    break;

  case SeekPos::End:
    if (offset < payloadSize())
    {
      _payload_position = static_cast<uint16_t>(_packet->payload_size - 1 - offset);
      return true;
    }
    break;

  default:
    break;
  }

  return false;
}


template <class Header>
const typename PacketStream<Header>::CrcType *PacketStream<Header>::crcPtr() const
{
  // CRC appears after the payload.
  const uint8_t *pos = reinterpret_cast<const uint8_t *>(_packet) + sizeof(Header) + payloadSize();
  return reinterpret_cast<const CrcType *>(pos);
}


template <class Header>
inline PacketStatus PacketStream<Header>::status() const
{
  return _status;
}

template <class Header>
inline Header &PacketStream<Header>::packet() const
{
  return *_packet;
}

template <class Header>
inline uint16_t PacketStream<Header>::tell() const
{
  return _payload_position;
}

template <class Header>
inline const uint8_t *PacketStream<Header>::payload() const
{
  return reinterpret_cast<const uint8_t *>(_packet) + sizeof(Header);
}


template <class Header>
inline void swap(PacketStream<Header> &a, PacketStream<Header> &b) noexcept
{
  a.swap(b);
}

// clang-format off
// NOLINTEND(cppcoreguidelines-pro-type-reinterpret-cast, cppcoreguidelines-pro-bounds-pointer-arithmetic)
// clang-format on

}  // namespace tes

#endif  // TES_CORE_HEADER_STREAM_H
