
//
// author: Kazys Stepanas
//
#pragma once

#include <3escore/CoreConfig.h>

#include <QTcpServer>
#include <QTcpSocket>

namespace tes
{
struct TcpSocketDetail
{
  std::unique_ptr<QTcpSocket> socket = {};
  int read_timeout = -1;
  int write_timeout = -1;

  TcpSocketDetail() = default;
  TcpSocketDetail(const TcpSocketDetail &other) = default;
  TcpSocketDetail(TcpSocketDetail &&other) = default;
  ~TcpSocketDetail() { delete socket; }

  TcpSocketDetail &operator=(const TcpSocketDetail &other) = default;
  TcpSocketDetail &operator=(TcpSocketDetail &&other) = default;
};

struct TcpListenSocketDetail
{
  QTcpServer listen_socket;
};
}  // namespace tes
