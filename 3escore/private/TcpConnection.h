//
// author: Kazys Stepanas
//
#pragma once

#include <3escore/Server.h>

#include <3escore/BaseConnection.h>

#include <memory>

namespace tes
{
class TcpSocket;

/// A TCP based implementation of a 3es @c Connection. Each @c TcpConnection represents a remote
/// client connection.
///
/// These connections are created by the @c TcpServer.
class TcpConnection final : public BaseConnection
{
public:
  /// Create a new connection using the given @p client_socket.
  /// @param client_socket The socket to communicate on.
  /// @param settings Various server settings to initialise with.
  TcpConnection(std::shared_ptr<TcpSocket> client_socket, const ServerSettings &settings);

  TcpConnection(const TcpConnection &other) = delete;

  /// Destructor.s
  ~TcpConnection() final;

  TcpConnection &operator=(const TcpConnection &other) = delete;

  /// Close the socket connection.
  void close() final;

  const char *address() const final;
  uint16_t port() const final;
  bool isConnected() const final;

protected:
  int writeBytes(const uint8_t *data, int byte_count) final;

private:
  std::shared_ptr<TcpSocket> _client;
};
}  // namespace tes
