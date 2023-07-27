//
// author: Kazys Stepanas
//
#ifndef TES_CORE_PACKET_H
#define TES_CORE_PACKET_H

#include "CoreConfig.h"

#include <cinttypes>

namespace tes
{
/// Marker value identifying a packet header in local Endian.
extern const uint32_t TES_CORE_API kPacketMarker;
/// Packet encoding major version local Endian.
extern const uint16_t TES_CORE_API kPacketVersionMajor;
/// Packet encoding minor version local Endian.
extern const uint16_t TES_CORE_API kPacketVersionMinor;

/// Packet decoding major compatibility version local Endian.
///
/// This is the minimum supported version number.
extern const uint16_t TES_CORE_API kPacketCompatibilityVersionMajor;
/// Packet decoding minor compatibility version local Endian.
///
/// This is the minimum supported version number.
extern const uint16_t TES_CORE_API kPacketCompatibilityVersionMinor;

/// Flag values for @c PacketHeader objects.
enum PacketFlag : uint8_t
{
  /// Marks a @c PacketHeader as missing its 16-bit CRC.
  PFNoCrc = (1u << 0u),
  // /// Indicates that the platform which wrote this data was big endian. In most cases this is
  // /// irrelevant as data items are generally written to big endian format. However, in some cases
  // /// the write operation may not be able to change to network endian form, in which case this
  // flag
  // /// indicates the source data endian format.
  // PF_PlatformBigEndian = (1u << 1u),
};

/// The header for an incoming 3ES data packet. All packet data, including payload bytes, must be in
/// network endian which is big endian.
///
/// All 3ES packets must being with this header, but may be followed by additional, the size of
/// which is indicated by @c payload_size . Note this is a 16-bit quantity, so limited to `0xffffu`.
///
/// A two byte CRC value is to appear immediately after the @p PacketHeader header and payload.
///
/// @note The @c payload_size bit width limit was chosen to keep the header size small. Initially
/// there were also some issues with the initial socket implementation causing some data loss due
/// to undersized buffers. The payload size can't be changed without invalidating the header format,
/// invalidating any previously captured data. However, a @c PacketFlag could be used to incidate
/// that a 4-byte payload size followes the @c PacketHeader . This change is reserved for future
/// implementation if need be. In particular larger packets may support better compression for
/// @c CollatedPacket .
///
/// @see The upper limit for the combined size of a packet is defined by
/// @c tes::kMaxPacketSize defined in @c PacketStream.h
struct TES_CORE_API PacketHeader
{
  uint32_t marker;         ///< Marker bytes. Identifies the packet start.
  uint16_t version_major;  ///< PacketHeader major version number. May be used to control decoding.
  uint16_t version_minor;  ///< PacketHeader minor version number. May be used to control decoding.
  /// Identifies the main packet receiver.
  uint16_t routing_id;
  /// Identifies the message ID or message type.
  uint16_t message_id;
  uint16_t payload_size;  ///< Size of the payload following this header.
  /// Offset from the end of this header to the payload.
  uint8_t payload_offset;
  /// @c PacketFlag values.
  uint8_t flags;
};
}  // namespace tes

#endif  // TES_CORE_PACKET_H
