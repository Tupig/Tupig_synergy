/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "net/QtNetworkTransport.h"

#include "base/Log.h"

//
// QtNetworkTransport
//

QtNetworkTransport::QtNetworkTransport()
{
  // TODO: Initialize QTcpSocket in Step 3
  LOG_DEBUG("QtNetworkTransport created (skeleton)");
}

QtNetworkTransport::~QtNetworkTransport()
{
  close();
}

void QtNetworkTransport::connect(const NetworkAddress &address)
{
  // TODO: Implement in Step 3 using QTcpSocket::connectToHost()
  LOG_WARN("QtNetworkTransport::connect not yet implemented");
  m_fatalError = true;
}

void QtNetworkTransport::bind(const NetworkAddress &address)
{
  // TODO: Implement in Step 3 using QTcpServer::listen()
  LOG_WARN("QtNetworkTransport::bind not yet implemented");
  m_fatalError = true;
}

void QtNetworkTransport::close()
{
  // TODO: Implement in Step 3
  m_connected = false;
}

bool QtNetworkTransport::isConnected() const
{
  return m_connected;
}

uint32_t QtNetworkTransport::read(void *buffer, uint32_t size)
{
  // TODO: Implement in Step 3 using QTcpSocket::read()
  Q_UNUSED(buffer);
  Q_UNUSED(size);
  return 0;
}

void QtNetworkTransport::write(const void *buffer, uint32_t size)
{
  // TODO: Implement in Step 3 using QTcpSocket::write()
  Q_UNUSED(buffer);
  Q_UNUSED(size);
}

void QtNetworkTransport::flush()
{
  // TODO: Implement in Step 3 using QTcpSocket::flush()
}

bool QtNetworkTransport::isReady() const
{
  // TODO: Implement in Step 3
  return false;
}

bool QtNetworkTransport::isFatal() const
{
  return m_fatalError;
}

uint32_t QtNetworkTransport::getSize() const
{
  // TODO: Implement in Step 3
  return 0;
}

ArchSocket QtNetworkTransport::getSocket() const
{
  // TODO: Implement in Step 3 - need to get native socket descriptor
  return nullptr;
}

void QtNetworkTransport::setSecurityLevel(SecurityLevel level)
{
  m_securityLevel = level;
}

SecurityLevel QtNetworkTransport::getSecurityLevel() const
{
  return m_securityLevel;
}

//
// QtTransportListenSocket
//

QtTransportListenSocket::QtTransportListenSocket()
{
  // TODO: Initialize QTcpServer in Step 3
  LOG_DEBUG("QtTransportListenSocket created (skeleton)");
}

QtTransportListenSocket::~QtTransportListenSocket() = default;

std::unique_ptr<INetworkTransport> QtTransportListenSocket::accept()
{
  // TODO: Implement in Step 3 using QTcpServer::nextPendingConnection()
  LOG_WARN("QtTransportListenSocket::accept not yet implemented");
  return nullptr;
}

ArchSocket QtTransportListenSocket::getSocket() const
{
  // TODO: Implement in Step 3
  return nullptr;
}
