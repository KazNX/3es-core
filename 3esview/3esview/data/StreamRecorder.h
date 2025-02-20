//
// Author: Kazys Stepanas
//
#pragma once

#include <3esview/ViewConfig.h>

#include <3esview/camera/Camera.h>

#include <3escore/FileConnection.h>

#include <filesystem>

namespace tes
{
class PacketReader;
struct ServerInfoMessage;
}  // namespace tes

namespace tes::view::camera
{
struct Camera;
}  // namespace tes::view::camera

namespace tes::view::data
{
class TES_VIEWER_API StreamRecorder
{
public:
  /// Tracks the state of the stream recorder.
  enum class State
  {
    /// Stream is open and awaiting the first snapshot.
    PendingSnapshot,
    /// Stream is recording.
    Recording,
    /// Stream is closed and may no longer be used.
    Closed
  };

  StreamRecorder(std::filesystem::path path, const ServerInfoMessage &server_info);
  ~StreamRecorder();

  StreamRecorder(const StreamRecorder &) = delete;
  StreamRecorder(StreamRecorder &&) = delete;

  StreamRecorder &operator=(const StreamRecorder &) = delete;
  StreamRecorder &operator=(StreamRecorder &&) = delete;

  /// Check if the stream is open.
  [[nodiscard]] bool isOpen() const { return _connection.isConnected(); }

  /// Query the stream state.
  [[nodiscard]] State status() const { return _status; }

  /// Access the underlying @c FileConnection object.
  [[nodiscard]] FileConnection &connection() { return _connection; }
  /// Access the underlying @c FileConnection object.
  [[nodiscard]] const FileConnection &connection() const { return _connection; }

  /// Query the recording path.
  /// @return The target path.
  const std::filesystem::path &path() const { return _path; }

  /// Mark a snapshot as having been taken.
  void markSnapshot()
  {
    _status = (_status == State::PendingSnapshot) ? State::Recording : _status;
  }

  /// Record the given packet if recording is active. Ignored when not recording.
  /// @param packet The packet to record.
  void recordPacket(const PacketReader &packet);

  /// Record the camera position as part of the current frame.
  /// @param camera The camera settings to record.
  void recordCamera(const camera::Camera &camera);

  /// Flush the current frame in the recording stream. Ignored when not recording.
  /// @param dt Frame elapsed time (seconds).
  void flush(float dt);

  /// Finalise and close the file stream.
  /// This stream cannot be reopened.
  void close();

private:
  /// Builds @c ServerSettings for initialising the @c FileConnection .
  /// @return The server settings structure.
  static ServerSettings serverSettings();

  /// Used to record and compress the data packets.
  FileConnection _connection;
  std::filesystem::path _path;
  State _status = State::PendingSnapshot;
};
}  // namespace tes::view::data
