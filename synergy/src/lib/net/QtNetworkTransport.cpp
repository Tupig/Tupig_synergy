/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "net/QtNetworkTransport.h"

#include "base/Log.h"
#include "net/NetworkAddress.h"

#include <QHostAddress>

#include <cstring>

//
// QtNetworkTransport
//

QtNetworkTransport::QtNetworkTransport()
{
  m_socket = new QTcpSocket(this);
  connectSignals();
  LOG_DEBUG("QtNetworkTransport: created");
}

QtNetworkTransport::QtNetworkTransport(QTcpSocket *socket)
  : m_socket(socket), m_connected(true)
{
  if (m_socket) {
    m_socket->setParent(this);
    connectSignals();
  }
  LOG_DEBUG("QtNetworkTransport: created (accepted)");
}

void QtNetworkTransport::connectSignals()
{
  if (!m_socket) {
    return;
  }
  QObject::connect(m_socket, &QTcpSocket::connected,
                   this, &QtNetworkTransport::onConnected);
  QObject::connect(m_socket, &QTcpSocket::disconnected,
                   this, &QtNetworkTransport::onDisconnected);
  QObject::connect(m_socket, &QTcpSocket::readyRead,
                   this, &QtNetworkTransport::onReadyRead);
  QObject::connect(m_socket, &QTcpSocket::bytesWritten,
                   this, &QtNetworkTransport::onBytesWritten);
  QObject::connect(m_socket, &QTcpSocket::errorOccurred,
                   this, &QtNetworkTransport::onErrorOccurred);
}

QtNetworkTransport::~QtNetworkTransport()
{
  close();
}

void QtNetworkTransport::connect(const NetworkAddress &address)
{
  if (!m_socket) {
    LOG_ERR("QtNetworkTransport: no socket");
    m_fatalError = true;
    return;
  }

  LOG_DEBUG("QtNetworkTransport: connecting to %s:%d",
            address.getHostname().c_str(), address.getPort());

  m_socket->connectToHost(
      QString::fromStdString(address.getHostname()),
      static_cast<quint16>(address.getPort())
  );
}

void QtNetworkTransport::bind(const NetworkAddress & /*address*/)
{
  LOG_WARN("QtNetworkTransport::bind not supported on client sockets");
}

void QtNetworkTransport::close()
{
  if (m_socket) {
    if (m_socket->isOpen()) {
      m_socket->close();
    }
  }
  m_connected = false;
  m_readBuffer.clear();
  m_writeBuffer.clear();
}

bool QtNetworkTransport::isConnected() const
{
  return m_connected && m_socket && m_socket->isOpen();
}

uint32_t QtNetworkTransport::read(void *buffer, uint32_t size)
{
  if (!m_connected || m_readBuffer.isEmpty()) {
    return 0;
  }

  uint32_t available = static_cast<uint32_t>(m_readBuffer.size());
  uint32_t toRead = std::min(size, available);

  std::memcpy(buffer, m_readBuffer.constData(), toRead);
  m_readBuffer.remove(0, static_cast<int>(toRead));

  return toRead;
}

void QtNetworkTransport::write(const void *buffer, uint32_t size)
{
  if (!m_connected || !m_socket) {
    LOG_ERR("QtNetworkTransport: not connected");
    return;
  }

  if (static_cast<size_t>(m_writeBuffer.size()) + size > m_writeQueueLimit) {
    LOG_WARN("QtNetworkTransport: write queue full, dropping data");
    m_fatalError = true;
    return;
  }

  m_writeBuffer.append(static_cast<const char *>(buffer), static_cast<int>(size));
  processWriteQueue();
}

void QtNetworkTransport::flush()
{
  if (m_socket && m_socket->isOpen()) {
    m_socket->flush();
  }
}

bool QtNetworkTransport::isReady() const
{
  return m_connected && !m_readBuffer.isEmpty();
}

bool QtNetworkTransport::isFatal() const
{
  return m_fatalError;
}

uint32_t QtNetworkTransport::getSize() const
{
  return static_cast<uint32_t>(m_readBuffer.size());
}

ArchSocket QtNetworkTransport::getSocket() const
{
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

QTcpSocket *QtNetworkTransport::getTcpSocket() const
{
  return m_socket;
}

void QtNetworkTransport::setWriteQueueLimit(size_t limit)
{
  m_writeQueueLimit = limit;
}

size_t QtNetworkTransport::getWriteQueueSize() const
{
  return static_cast<size_t>(m_writeBuffer.size());
}

void QtNetworkTransport::onConnected()
{
  LOG_INFO("QtNetworkTransport: connected");
  m_connected = true;
  m_fatalError = false;
}

void QtNetworkTransport::onDisconnected()
{
  LOG_INFO("QtNetworkTransport: disconnected");
  m_connected = false;
}

void QtNetworkTransport::onReadyRead()
{
  if (!m_socket) {
    return;
  }

  QByteArray data = m_socket->readAll();
  if (!data.isEmpty()) {
    m_readBuffer.append(data);
  }
}

void QtNetworkTransport::onBytesWritten(qint64 bytes)
{
  if (bytes > 0 && m_writeBuffer.size() >= bytes) {
    m_writeBuffer.remove(0, static_cast<int>(bytes));
  }
}

void QtNetworkTransport::onErrorOccurred(QAbstractSocket::SocketError socketError)
{
  if (m_socket) {
    LOG_ERR("QtNetworkTransport: error: %s",
            m_socket->errorString().toStdString().c_str());
  }

  if (socketError == QAbstractSocket::ConnectionRefusedError ||
      socketError == QAbstractSocket::SocketTimeoutError) {
    LOG_WARN("QtNetworkTransport: non-fatal error, can retry");
  } else {
    m_fatalError = true;
  }
}

void QtNetworkTransport::processWriteQueue()
{
  if (!m_socket || m_writeBuffer.isEmpty()) {
    return;
  }

  qint64 written = m_socket->write(m_writeBuffer);
  if (written > 0) {
    m_writeBuffer.remove(0, static_cast<int>(written));
  }
}

//
// QtTransportListenSocket
//

QtTransportListenSocket::QtTransportListenSocket()
{
  m_server = new QTcpServer(this);
  LOG_DEBUG("QtTransportListenSocket: created");
}

QtTransportListenSocket::~QtTransportListenSocket()
{
  if (m_server && m_server->isListening()) {
    m_server->close();
  }
}

std::unique_ptr<INetworkTransport> QtTransportListenSocket::accept()
{
  if (!m_server || !m_server->hasPendingConnections()) {
    return nullptr;
  }

  QTcpSocket *tcpSocket = m_server->nextPendingConnection();
  if (!tcpSocket) {
    return nullptr;
  }

  LOG_DEBUG("QtTransportListenSocket: accepted connection");
  return std::make_unique<QtNetworkTransport>(tcpSocket);
}

ArchSocket QtTransportListenSocket::getSocket() const
{
  return nullptr;
}

void QtTransportListenSocket::bindAndListen(const NetworkAddress &address)
{
  if (!m_server) {
    LOG_ERR("QtTransportListenSocket: no server");
    return;
  }

  QHostAddress addr = address.getHostname().empty()
      ? QHostAddress::Any
      : QHostAddress(QString::fromStdString(address.getHostname()));

  if (!m_server->listen(addr, static_cast<quint16>(address.getPort()))) {
    LOG_ERR("QtTransportListenSocket: failed to listen: %s",
            m_server->errorString().toStdString().c_str());
  } else {
    LOG_INFO("QtTransportListenSocket: listening on %s:%d",
             address.getHostname().c_str(), address.getPort());
  }
}

int QtTransportListenSocket::getServerSocketDescriptor() const
{
  if (m_server) {
    return m_server->socketDescriptor();
  }
  return -1;
}
