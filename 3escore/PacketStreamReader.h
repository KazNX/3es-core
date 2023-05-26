//
// author: Kazys Stepanas
//
#ifndef TES_CORE_PACKET_STREAM_READER_H
#define TES_CORE_PACKET_STREAM_READER_H

#include "CoreConfig.h"

#include "PacketHeader.h"

#include <array>
#include <cinttypes>
#include <istream>
#include <memory>
#include <vector>

namespace tes
{
struct PacketHeader;

/// A utility class which reads packets from a std::istream .
///
/// This collects bytes until a full packet is collected whenever
/// @c extractPacket() is called, provided there are sufficient bytes available.
/// A @c PacketReader is still required to decode the contents of the resulting
/// @c PacketHeader data.
class TES_CORE_API PacketStreamReader
{
public:
  /// Status values for @c extractPacket()
  enum class Status
  {
    /// Operation successful.
    Success,
    /// There is no stream to read from.
    NoStream,
    /// No data available
    Unavailable,
    /// Operation successful, but some data where skiped/dropped.
    Dropped,
    /// A packet header marker was found, but the packet is incomplete.
    Incomplete,
    /// The end of the stream has been reached. Nothing more is available.
    End,
  };

  /// Return value for @c extractPacket() .
  struct ExtractedPacket
  {
    /// A borrowed pointer to the extracted packet. Valid until the next @c extractPacket() call.
    /// Null if there are no packets available.
    const PacketHeader *header = nullptr;
    /// Status indicator of the operation state.
    Status status = Status::NoStream;
    /// File position at which the @c header starts.
    std::istream::pos_type pos = {};
  };

  /// Default constructor. The resulting reader is invalid. Use @c setStream() to initialise.
  PacketStreamReader();
  /// Construct a stream reader for the given stream.
  /// @param stream The stream to read.
  /// @todo It doens't make sense to use a @c std::shared_ptr to a @c std::istream . Convert to move
  /// semantics.
  PacketStreamReader(std::shared_ptr<std::istream> stream);

  PacketStreamReader(const PacketStreamReader &) = delete;

  ~PacketStreamReader();

  PacketStreamReader &operator=(const PacketStreamReader &) = delete;

  /// Check if the stream is ok for more reading.
  /// @return True if ok to read on.
  [[nodiscard]] bool isOk() const { return _stream && (!_buffer.empty() || _stream->good()); }

  /// Check if the stream is at the end of file.
  ///
  /// Note that this may report false, and allow @c extractPacket() to return @c Status::End before
  /// this value reports true.
  ///
  /// @return True if at end of file.
  [[nodiscard]] bool isEof() const { return !_stream || (_buffer.empty() && _stream->eof()); }

  /// (Re)set the stream to read from.
  /// @param stream The stream to read from.
  void setStream(std::shared_ptr<std::istream> stream);
  /// Get the stream in use.
  /// @return The current stream - may be null.
  [[nodiscard]] std::shared_ptr<std::istream> stream() const { return _stream; }

  /// Try extract the next packet from the stream. The packet pointer remains
  /// valid until the next call to @c extractPacket(). This object retains the
  /// ownership.
  ///
  /// Note there is no version check on the packet. The caller should check the
  /// packet version for compatibility.
  ///
  /// @return The next packet or null on failure and a @c Status code.
  ExtractedPacket extractPacket();

  /// Seek to the given stream position.
  ///
  /// This clears the current data buffer, invalidating results from @c extractPacket().
  ///
  /// @param position The stream byte offset to seek to.
  void seek(std::istream::pos_type position);

private:
  size_t readMore(size_t more_count);
  bool checkMarker(std::vector<uint8_t> &buffer, size_t i);
  /// Consume the packet at the head of the buffer (if valid and able).
  void consume();

  /// Calculate the expected packet size for the packet at the head of the buffer.
  ///
  /// Only valid to call when we have a verified header at the _buffer start.
  /// @return The expected packet size including payload as indicated by the packet header.
  size_t calcExpectedSize();

  std::shared_ptr<std::istream> _stream;
  std::array<uint8_t, sizeof(tes::kPacketMarker)> _marker_bytes = {};
  std::vector<uint8_t> _buffer;
  size_t _chunk_size = 1024u;
  std::istream::pos_type _current_packet_pos = {};
};
}  // namespace tes

#endif  // TES_CORE_PACKET_STREAM_READER_H
