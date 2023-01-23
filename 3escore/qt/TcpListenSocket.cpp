//
// author: Kazys Stepanas
//
#include "TcpListenSocket.h"

#include "TcpSocket.h"

#include "TcpDetail.h"

#include <QHostAddress>

#include <cstring>

using namespace tes;

TcpListenSocket::TcpListenSocket()
  : _detail(new TcpListenSocketDetail)
{}


TcpListenSocket::~TcpListenSocket()
{
  close();
  delete _detail;
}


uint16_t TcpListenSocket::port() const
{
  return _detail->listenSocket.serverPort();
}


bool TcpListenSocket::listen(unsigned short port)
{
  if (isListening())
  {
    return false;
  }

  return _detail->listenSocket.listen(QHostAddress::Any, port);
}


void TcpListenSocket::close()
{
  if (isListening())
  {
    _detail->listenSocket.close();
  }
}


bool TcpListenSocket::isListening() const
{
  return _detail->listenSocket.isListening();
}


TcpSocket *TcpListenSocket::accept(unsigned timeoutMs)
{
  if (!_detail->listenSocket.waitForNewConnection(timeoutMs))
  {
    return nullptr;
  }

  if (!_detail->listenSocket.hasPendingConnections())
  {
    return nullptr;
  }

  QTcpSocket *newSocket = _detail->listenSocket.nextPendingConnection();
  if (!newSocket)
  {
    return nullptr;
  }
  TcpSocketDetail *clientDetail = new TcpSocketDetail;
  clientDetail->socket = newSocket;
  return new TcpSocket(clientDetail);
}


void TcpListenSocket::releaseClient(TcpSocket *client)
{
  if (client)
  {
    client->close();
    delete client;
  }
}
