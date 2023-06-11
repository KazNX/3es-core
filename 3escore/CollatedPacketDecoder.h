//
// author: Kazys Stepanas
//
#ifndef TES_CORE_COLLATED_PACKET_DECODER_H
#define TES_CORE_COLLATED_PACKET_DECODER_H

#include "CoreConfig.h"

//
#include "Connection.h"
#include "PacketHeader.h"

#include <memory>

namespace tes
{
struct PacketHeader;

struct CollatedPacketDecoderDetail;

/// A utility class for decoding @c CollatedPacketMessage packets.
///
/// These are packets with a message type of @c MtCollatedPacket containing a
/// @c CollatedPacketMessage followed by a payload containing additional message packets,
/// optionally compressed using GZip compression. Such packets may be generated using the
/// @c CollatedPacket class.
///
/// While the decoder supports decoding @c CollatedPacketMessage, it can handle other
/// mesage packets by simply returning the supplied packet as is. This allows the usage
/// of the decoder to be content agnostic.
///
/// Typical usage is illustrated bellow:
/// @code
/// void readPackets(TcpSocket &socket)
/// {
///   std::vector<uint8_t> readBuffer(tes::kMaxPacketSize);
///   PacketBuffer packetBuffer;
///   CollatedPacketDecoder decoder;
///   std::vector<uint8_t> rawBuffer;
///
///   /// Read from the socket.
///   while (socket.readAvailable(readBuffer.data(), readBuffer.size() >= 0)
///   {
///     // Add new data to the packet buffer.
///     packetBuffer.addBytes(readBuffer.data(), readCount);
///
///     /// Process new packets.
///     while (PacketHeader *primaryPacket = packetBuffer.extractPacket(rawBuffer))
///     {
///       // Extract collated packets. This will either decode a collated packet or
///       // return the same packet header just passed in.
///       decoder.setPacket(primaryPacket);
///       while (const PacketHeader *packetHeader = decoder.next())
///       {
///         processPacket(packetHeader);
///       }
///     }
///   }
/// }
/// @endcode
class TES_CORE_API CollatedPacketDecoder
{
public:
  /// Create a new packet decoder, optionally starting with the given @p packet.
  /// @param packet The first packet to decode.
  CollatedPacketDecoder(const PacketHeader *packet = nullptr);

  CollatedPacketDecoder(const CollatedPacketDecoder &) = delete;
  CollatedPacketDecoder(CollatedPacketDecoder &&) = delete;

  /// Destructor.
  ~CollatedPacketDecoder();

  CollatedPacketDecoder &operator=(const CollatedPacketDecoder &) = delete;
  CollatedPacketDecoder &operator=(CollatedPacketDecoder &&) = delete;

  /// Returns the number of bytes which have been decoded from the current primary packet.
  /// @return The number of decompressed bytes decoded.
  [[nodiscard]] unsigned decodedBytes() const;

  /// Returns the target number of bytes to decode from the current primary packet.
  /// @return The total number of bytes to decompress and decode from the primary packet.
  [[nodiscard]] unsigned targetBytes() const;

  /// True if the decoder is currently decoding a packet. This turns false after the last
  /// of the current packets is extracted from @c next().
  /// @return true while decoding a packet.
  [[nodiscard]] bool decoding() const;

  /// Set the primary packet to decode. This may be any packet type, but only a
  /// @c CollatedPacketMessage will generate multiple subsequent packets via @c next().
  /// Other packet types will be returned as is from the first call to @c next().
  ///
  /// The memory for @p packet must persist until either @c next() returns null or the
  /// decoder object goes out of scope.
  ///
  /// @param packet The primary packet to decode.
  /// @return True if decoding of @p packet successfully starts. False if @p packet is null
  ///   or we fail to read the content of a @c CollatedPacketMessage message.
  bool setPacket(const PacketHeader *packet);

  /// Extract the next packet from the primary packet. This should be called iteratively
  /// until it returns null. Multiple packets will be extracted by this call when the primary
  /// packet, set via @c setPacket(), is a @c CollatedPacketMessage. Otherwise the primary @c
  /// PacketHeader is returned (same as that passed to @c setPacket()) followed by a null result.
  ///
  /// The returned packet remains valid until the next call to @c next(), a new primary packet is
  /// set or this object goes out of scope.
  ///
  /// @return The next extracted packet or null when there are no more available.
  [[nodiscard]] const PacketHeader *next();

private:
  std::unique_ptr<CollatedPacketDecoderDetail> _detail;
};
}  // namespace tes

#endif  // TES_CORE_COLLATED_PACKET_DECODER_H
