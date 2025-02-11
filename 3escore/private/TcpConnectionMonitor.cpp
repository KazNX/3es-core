//
// author: Kazys Stepanas
//
#include "TcpConnectionMonitor.h"

#include "TcpConnection.h"
#include "TcpServer.h"

#include <3escore/CoreUtil.h>
#include <3escore/FileConnection.h>
#include <3escore/Log.h>
#include <3escore/TcpListenSocket.h>
#include <3escore/TcpSocket.h>

#include <chrono>
#include <mutex>

namespace tes
{
TcpConnectionMonitor::TcpConnectionMonitor(TcpServer &server)
  : _server(server)
{}


TcpConnectionMonitor::~TcpConnectionMonitor()
{
  stop();
  join();
  _listen.reset();
  _thread.reset();
}


TcpConnectionError TcpConnectionMonitor::lastErrorCode() const
{
  return static_cast<TcpConnectionError>(_error_code.load());
}


TcpConnectionError TcpConnectionMonitor::clearErrorCode()
{
  const TcpConnectionError last_error = static_cast<TcpConnectionError>(_error_code.load());
  _error_code = static_cast<int>(TcpConnectionError::None);
  return last_error;
}


uint16_t TcpConnectionMonitor::port() const
{
  return _listen_port;
}


bool TcpConnectionMonitor::start(ConnectionMode mode)
{
  if (mode == ConnectionMode::None || _mode != ConnectionMode::None && mode != _mode)
  {
    return false;
  }

  if (mode == _mode)
  {
    return true;
  }

  switch (mode)
  {
  case ConnectionMode::Synchronous:
    if (listen())
    {
      _running = true;
      _mode = ConnectionMode::Synchronous;
    }
    else
    {
      _error_code = static_cast<int>(TcpConnectionError::ListenFailure);
      stopListening();
    }
    break;

  case ConnectionMode::Asynchronous: {
    join();  // Pointer may linger after quit.
    _thread = std::make_unique<std::thread>([this]() { monitorThread(); });
    // Wait for the thread to start. We look for _running or an _error_code.
    auto wait_start = std::chrono::steady_clock::now();
    unsigned elapsed_ms = 0;
    while (!_running && !_error_code && elapsed_ms <= _server.settings().async_timeout_ms)
    {
      std::this_thread::yield();
      elapsed_ms = int_cast<unsigned>(std::chrono::duration_cast<std::chrono::milliseconds>(
                                        std::chrono::steady_clock::now() - wait_start)
                                        .count());
    }

    // Running will be true if the thread started ok.
    if (_running)
    {
      _mode = ConnectionMode::Asynchronous;
    }

    if (!_running && !_error_code && elapsed_ms >= _server.settings().async_timeout_ms)
    {
      _error_code = static_cast<int>(TcpConnectionError::Timeout);
    }
    break;
  }

  default:
    break;
  }

  return _mode != ConnectionMode::None;
}


void TcpConnectionMonitor::stop()
{
  switch (_mode)
  {
  case ConnectionMode::Synchronous:
    _running = false;
    stopListening();
    _mode = ConnectionMode::None;
    break;

  case ConnectionMode::Asynchronous:
    _quit_flag = true;
    break;

  default:
    break;
  }
}


void TcpConnectionMonitor::join()
{
  if (_thread)
  {
    if (!_quit_flag && (_mode == ConnectionMode::Asynchronous || _mode == ConnectionMode::None))
    {
      log::error("ConnectionMonitor::join() called on asynchronous connection monitor without "
                 "calling stop()");
    }
    _thread->join();
    _thread.reset();
  }
}


bool TcpConnectionMonitor::isRunning() const
{
  return _running;
}


ConnectionMode TcpConnectionMonitor::mode() const
{
  return _mode;
}


int TcpConnectionMonitor::waitForConnection(unsigned timeout_ms)
{
  std::unique_lock<Lock> lock(_connection_lock);
  if (!_connections.empty())
  {
    return int_cast<int>(_connections.size());
  }
  lock.unlock();

  // Wait for start.
  if (mode() == ConnectionMode::Asynchronous)
  {
    while (!isRunning() && mode() != ConnectionMode::None)
    {}
  }

  // Update connections if required.
  auto start_time = std::chrono::steady_clock::now();
  bool timedout = false;
  int connection_count = 0;
  while (isRunning() && !timedout && connection_count == 0)
  {
    if (mode() == ConnectionMode::Synchronous)
    {
      monitorConnections();
    }
    else
    {
      std::this_thread::yield();
    }
    timedout = std::chrono::duration_cast<std::chrono::milliseconds>(
                 std::chrono::steady_clock::now() - start_time)
                 .count() >= timeout_ms;
    lock.lock();
    connection_count = int_cast<int>(_connections.size());
    lock.unlock();
  }

  return connection_count;
}


void TcpConnectionMonitor::monitorConnections()
{
  // Lock for connection expiry.
  std::unique_lock<Lock> lock(_connection_lock);

  // Expire lost connections.
  for (auto iter = _connections.begin(); iter != _connections.end();)
  {
    auto connection = *iter;
    if (connection->isConnected())
    {
      ++iter;
    }
    else
    {
      _expired.push_back(connection);
      iter = _connections.erase(iter);
    }
  }

  // Unlock while we check for new connections.
  lock.unlock();

  // Look for new connections.
  if (_listen)
  {
    if (auto new_socket = _listen->accept(0))
    {
      // Options to try and reduce socket latency.
      // Attempt to prevent periodic latency on osx.
      new_socket->setNoDelay(true);
      new_socket->setWriteTimeout(0);
      new_socket->setReadTimeout(0);
#ifdef __apple__
      // On OSX, set send buffer size. Not sure automatic sizing is working.
      // Remove this code if it is.
      new_socket->setSendBufferSize(1024 * 1024);
#endif  // __apple__

      auto new_connection = std::make_shared<TcpConnection>(new_socket, _server.settings());
      // Lock for new connection.
      lock.lock();
      _connections.push_back(new_connection);
      lock.unlock();
    }
  }
}


std::shared_ptr<Connection> TcpConnectionMonitor::openFileStream(const std::string &file_path)
{
  auto new_connection = std::make_shared<FileConnection>(file_path, _server.settings());
  if (!new_connection->isConnected())
  {
    return nullptr;
  }

  const std::unique_lock<Lock> lock(_connection_lock);
  _connections.push_back(new_connection);
  return new_connection;
}


void TcpConnectionMonitor::setConnectionCallback(void (*callback)(Server &, Connection &, void *),
                                                 void *user)
{
  _on_new_connection = [callback, user](Server &server, Connection &connection) {
    (*callback)(server, connection, user);
  };
}


void TcpConnectionMonitor::setConnectionCallback(
  const std::function<void(Server &, Connection &)> &callback)
{
  _on_new_connection = callback;
}


const std::function<void(Server &, Connection &)> &TcpConnectionMonitor::connectionCallback() const
{
  return _on_new_connection;
}


void TcpConnectionMonitor::commitConnections()
{
  std::unique_lock<Lock> lock(_connection_lock);
  _server.updateConnections(_connections, _on_new_connection);

  lock.unlock();

  // Delete expired connections.
  _expired.clear();
}


bool TcpConnectionMonitor::listen()
{
  if (_listen)
  {
    return true;
  }

  _listen = std::make_unique<TcpListenSocket>();

  bool listening = false;

  uint16_t port = _server.settings().listen_port;
  while (!listening && port <= _server.settings().listen_port + _server.settings().port_range)
  {
    listening = _listen->listen(port++);
  }

  _listen_port = (listening) ? _listen->port() : 0;

  return listening;
}


void TcpConnectionMonitor::stopListening()
{
  _listen_port = 0;

  // Close all connections.
  for (const auto &con : _connections)
  {
    con->close();
  }

  _listen.reset();
}


void TcpConnectionMonitor::monitorThread()
{
  if (!listen())
  {
    _error_code = static_cast<int>(TcpConnectionError::ListenFailure);
    stopListening();
    return;
  }
  _running = true;

  const unsigned sleep_ms = 50;
  while (!_quit_flag)
  {
    monitorConnections();
    std::this_thread::sleep_for(std::chrono::milliseconds(sleep_ms));
  }

  _running = false;
  stopListening();
  _mode = ConnectionMode::None;
}
}  // namespace tes
