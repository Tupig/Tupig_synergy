/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "net/LegacyNetworkTransport.h"

#include "net/IDataSocket.h"
#include "net/IListenSocket.h"

//
// LegacyNetworkTransport
//

LegacyNetworkTransport::LegacyNetworkTransport(IDataSocket *socket, IEventQueue *events)
    : m_dataSocket(socket),
      m_events(events)
{
  // do nothing
}

LegacyNetworkTransport::LegacyNetworkTransport(IListenSocket *socket, IEventQueue *events)
    : m_listenSocket(socket),
      m_events(events)
{
  // do nothing
}

LegacyNetworkTransport::~LegacyNetworkTransport()
{
  close();
}

void LegacyNetworkTransport::connect(const NetworkAddress &address)
{
  if (m_dataSocket) {
    m_dataSocket->connect(address);
  }
}

void LegacyNetworkTransport::bind(const NetworkAddress &address)
{
  if (m_listenSocket) {
    m_listenSocket->bind(address);
  } else if (m_dataSocket) {
    m_dataSocket->bind(address);
  }
}

void LegacyNetworkTransport::close()
{
  if (m_dataSocket) {
    m_dataSocket->close();
  }
  if (m_listenSocket) {
    m_listenSocket->close();
  }
}

bool LegacyNetworkTransport::isConnected() const
{
  // For data sockets, check if we have a valid socket
  // The actual connection state is managed by the event system
  return m_dataSocket != nullptr;
}

uint32_t LegacyNetworkTransport::read(void *buffer, uint32_t size)
{
  if (m_dataSocket) {
    return m_dataSocket->read(buffer, size);
  }
  return 0;
}

void LegacyNetworkTransport::write(const void *buffer, uint32_t size)
{
  if (m_dataSocket) {
    m_dataSocket->write(buffer, size);
  }
}

void LegacyNetworkTransport::flush()
{
  if (m_dataSocket) {
    m_dataSocket->flush();
  }
}

bool LegacyNetworkTransport::isReady() const
{
  if (m_dataSocket) {
    return m_dataSocket->isReady();
  }
  return false;
}

bool LegacyNetworkTransport::isFatal() const
{
  if (m_dataSocket) {
    return m_dataSocket->isFatal();
  }
  return true;
}

uint32_t LegacyNetworkTransport::getSize() const
{
  if (m_dataSocket) {
    return m_dataSocket->getSize();
  }
  return 0;
}

ArchSocket LegacyNetworkTransport::getSocket() const
{
  // Legacy sockets don't expose their raw socket directly
  // This will be addressed in Step 3 when we implement QtTcpTransport
  return nullptr;
}

void LegacyNetworkTransport::setSecurityLevel(SecurityLevel level)
{
  m_securityLevel = level;
}

SecurityLevel LegacyNetworkTransport::getSecurityLevel() const
{
  return m_securityLevel;
}

IDataSocket *LegacyNetworkTransport::getSocketImpl() const
{
  return m_dataSocket;
}

//
// LegacyTransportListenSocket
//

LegacyTransportListenSocket::LegacyTransportListenSocket(IListenSocket *socket)
    : m_socket(socket)
{
  // do nothing
}

LegacyTransportListenSocket::~LegacyTransportListenSocket() = default;

std::unique_ptr<INetworkTransport> LegacyTransportListenSocket::accept()
{
  if (m_socket) {
    auto dataSocket = m_socket->accept();
    if (dataSocket) {
      // We need to get the event queue from somewhere
      // For now, pass nullptr - this will be fixed when we integrate
      return std::make_unique<LegacyNetworkTransport>(dataSocket.release(), nullptr);
    }
  }
  return nullptr;
}

ArchSocket LegacyTransportListenSocket::getSocket() const
{
  // Legacy listen sockets don't expose their raw socket directly
  return nullptr;
}
