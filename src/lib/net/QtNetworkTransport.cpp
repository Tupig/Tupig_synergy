/*
 * Synergy -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
 * SPDX-FileCopyrightText: (C) 2026 TuPig
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "net/QtNetworkTransport.h"

#include "base/Log.h"
#include "common/Settings.h"
#include "net/NetworkAddress.h"

#include <QFile>
#include <QHostAddress>
#include <QSslCertificate>
#include <QSslConfiguration>
#include <QSslKey>
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#include <QSslServer>
#endif

#include <cstring>

namespace {

bool loadPemIdentity(QSslConfiguration &config, const QString &path)
{
  QFile file(path);
  if (!file.open(QIODevice::ReadOnly)) {
    LOG_ERR("QtNetworkTransport: cannot open certificate \"%s\"", path.toStdString().c_str());
    return false;
  }

  const QByteArray pem = file.readAll();
  const QList<QSslCertificate> certs = QSslCertificate::fromData(pem, QSsl::Pem);
  if (certs.isEmpty()) {
    LOG_ERR("QtNetworkTransport: no certificate in \"%s\"", path.toStdString().c_str());
    return false;
  }

  QSslKey key(pem, QSsl::Rsa, QSsl::Pem, QSsl::PrivateKey);
  if (key.isNull()) {
    key = QSslKey(pem, QSsl::Ec, QSsl::Pem, QSsl::PrivateKey);
  }
  if (key.isNull()) {
    LOG_ERR("QtNetworkTransport: no private key in \"%s\"", path.toStdString().c_str());
    return false;
  }

  config.setLocalCertificate(certs.first());
  if (certs.size() > 1) {
    config.setCaCertificates(certs.mid(1));
  }
  config.setPrivateKey(key);
  return true;
}

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
//! Qt 5 has no QSslServer; upgrade every accepted socket manually.
/*!
`incomingConnection` adopts the raw descriptor into a QSslSocket, queues it
and starts the server-side handshake.  QSslSocket requires a QCoreApplication
event loop, which the process already has.
*/
class Qt5TlsTcpServer : public QTcpServer
{
public:
  using QTcpServer::QTcpServer;

  QSslConfiguration sslConfiguration;

protected:
  void incomingConnection(qintptr socketDescriptor) override
  {
    auto *socket = new QSslSocket(this);
    if (!socket->setSocketDescriptor(socketDescriptor)) {
      LOG_ERR("QtTransportListenSocket: cannot adopt incoming socket");
      delete socket;
      return;
    }
    socket->setSslConfiguration(sslConfiguration);
    // The handshake must tolerate self-signed client certificates (the raw
    // stack's cert callback behaves the same); trust is fingerprint-based.
    QObject::connect(
        socket, qOverload<const QList<QSslError> &>(&QSslSocket::sslErrors), socket,
        [socket](const QList<QSslError> &errors) {
          for (const auto &err : errors) {
            LOG_DEBUG("QtTransportListenSocket: ignoring SSL error: %s", err.errorString().toStdString().c_str());
          }
          socket->ignoreSslErrors();
        }
    );
    addPendingConnection(socket);
    socket->startServerEncryption();
  }
};
#endif

} // namespace

//
// QtNetworkTransport
//

QtNetworkTransport::QtNetworkTransport(SecurityLevel securityLevel) : m_securityLevel(securityLevel)
{
  createSocket();
  connectSignals();
  LOG_DEBUG("QtNetworkTransport: created (tls=%d)", wantsTls() ? 1 : 0);
}

QtNetworkTransport::QtNetworkTransport(QTcpSocket *socket, SecurityLevel securityLevel)
    : m_socket(socket),
      m_connected(socket != nullptr && socket->state() == QAbstractSocket::ConnectedState),
      m_securityLevel(securityLevel)
{
  if (m_socket) {
    m_socket->setParent(this);
    if (auto *ssl = qobject_cast<QSslSocket *>(m_socket)) {
      if (wantsTls() && ssl->isEncrypted()) {
        m_connected = true;
        // QSslServer (Qt 6) hands out sockets only after the TLS handshake,
        // so encrypted() has already fired by the time we can subscribe.
        // Replay the connected notification once subscribers are wired up.
        QMetaObject::invokeMethod(this, [this] { Q_EMIT transportConnected(); }, Qt::QueuedConnection);
      }
    }
    connectSignals();
  }
  LOG_DEBUG("QtNetworkTransport: created (accepted, tls=%d)", wantsTls() ? 1 : 0);
}

bool QtNetworkTransport::wantsTls() const
{
  return m_securityLevel == SecurityLevel::Encrypted || m_securityLevel == SecurityLevel::PeerAuth;
}

void QtNetworkTransport::createSocket()
{
  if (wantsTls()) {
    auto *ssl = new QSslSocket(this);
    if (m_securityLevel == SecurityLevel::PeerAuth) {
      ssl->setPeerVerifyMode(QSslSocket::VerifyPeer);
    } else {
      // App-level fingerprint checks match SecureSocket Encrypted mode.
      ssl->setPeerVerifyMode(QSslSocket::VerifyNone);
    }
    m_socket = ssl;
  } else {
    m_socket = new QTcpSocket(this);
  }
}

bool QtNetworkTransport::applyCertificate(QSslSocket *ssl, bool /*isServer*/)
{
  if (ssl == nullptr) {
    return false;
  }

  const QString path = Settings::value(Settings::Security::Certificate).toString();
  if (path.isEmpty()) {
    LOG_ERR("QtNetworkTransport: security/certificate setting is empty");
    return false;
  }

  QSslConfiguration config = ssl->sslConfiguration();
  if (!loadPemIdentity(config, path)) {
    return false;
  }
  ssl->setSslConfiguration(config);
  return true;
}

void QtNetworkTransport::connectSignals()
{
  if (!m_socket) {
    return;
  }

  if (auto *ssl = qobject_cast<QSslSocket *>(m_socket)) {
    QObject::connect(ssl, &QSslSocket::encrypted, this, &QtNetworkTransport::onConnected);
    // Qt5: sslErrors is both a signal and a const getter — qOverload disambiguates.
    QObject::connect(
        ssl, qOverload<const QList<QSslError> &>(&QSslSocket::sslErrors), this, &QtNetworkTransport::onSslErrors
    );
  } else {
    QObject::connect(m_socket, &QTcpSocket::connected, this, &QtNetworkTransport::onConnected);
  }

  QObject::connect(m_socket, &QTcpSocket::disconnected, this, &QtNetworkTransport::onDisconnected);
  QObject::connect(m_socket, &QTcpSocket::readyRead, this, &QtNetworkTransport::onReadyRead);
  QObject::connect(m_socket, &QTcpSocket::bytesWritten, this, &QtNetworkTransport::onBytesWritten);
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
  QObject::connect(m_socket, &QTcpSocket::errorOccurred, this, &QtNetworkTransport::onErrorOccurred);
#else
  // errorOccurred was added in 5.15; Qt 5.13 (RHEL 8 floor) still uses error().
  QObject::connect(
      m_socket, static_cast<void (QAbstractSocket::*)(QAbstractSocket::SocketError)>(&QAbstractSocket::error), this,
      &QtNetworkTransport::onErrorOccurred
  );
#endif
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

  const QString host = QString::fromStdString(address.getHostname());
  const quint16 port = static_cast<quint16>(address.getPort());

  LOG_DEBUG(
      "QtNetworkTransport: connecting to %s:%d (tls=%d)", address.getHostname().c_str(), address.getPort(),
      wantsTls() ? 1 : 0
  );

  if (auto *ssl = qobject_cast<QSslSocket *>(m_socket); ssl != nullptr && wantsTls()) {
    if (!applyCertificate(ssl, false)) {
      m_fatalError = true;
      return;
    }
    ssl->connectToHostEncrypted(host, port);
  } else {
    m_socket->connectToHost(host, port);
  }
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
  LOG_INFO("QtNetworkTransport: connected%s", wantsTls() ? " (TLS)" : "");
  m_connected = true;
  m_fatalError = false;
  Q_EMIT transportConnected();
}

void QtNetworkTransport::onDisconnected()
{
  LOG_INFO("QtNetworkTransport: disconnected");
  m_connected = false;
  Q_EMIT transportDisconnected();
}

void QtNetworkTransport::onReadyRead()
{
  if (!m_socket) {
    return;
  }

  QByteArray data = m_socket->readAll();
  if (!data.isEmpty()) {
    m_readBuffer.append(data);
    Q_EMIT transportDataReady();
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
    LOG_ERR("QtNetworkTransport: error: %s", m_socket->errorString().toStdString().c_str());
  }

  if (socketError == QAbstractSocket::ConnectionRefusedError || socketError == QAbstractSocket::SocketTimeoutError) {
    LOG_WARN("QtNetworkTransport: non-fatal error, can retry");
  } else {
    m_fatalError = true;
  }
  Q_EMIT transportError(m_socket ? m_socket->errorString() : QString(), m_fatalError);
}

void QtNetworkTransport::onSslErrors(const QList<QSslError> &errors)
{
  // Chain/self-signed errors are tolerated like the raw stack's cert verify
  // callback: the handshake proceeds and the fingerprint database is the
  // actual trust gate (checked by the adapter once encrypted).
  if (auto *ssl = qobject_cast<QSslSocket *>(m_socket)) {
    for (const auto &err : errors) {
      LOG_DEBUG("QtNetworkTransport: ignoring SSL error: %s", err.errorString().toStdString().c_str());
    }
    ssl->ignoreSslErrors();
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

QtTransportListenSocket::QtTransportListenSocket(SecurityLevel securityLevel) : m_securityLevel(securityLevel)
{
  // QSslServer is Qt 6+; Qt 5 (RHEL/Rocky, and Debian/Ubuntu legs that fall
  // back when system Qt 6 is below the project floor) only has QTcpServer.
  // TLS listen on Qt 5 is rejected in configureSslServer() below.
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
  if (wantsTls()) {
    auto *sslServer = new QSslServer(this);
    // Self-signed client certificates must not abort the handshake (raw-stack
    // parity); the fingerprint database is the trust gate.
    QObject::connect(
        sslServer, &QSslServer::sslErrors, sslServer,
        [](QSslSocket *socket, const QList<QSslError> &errors) {
          for (const auto &err : errors) {
            LOG_DEBUG("QtTransportListenSocket: ignoring SSL error: %s", err.errorString().toStdString().c_str());
          }
          socket->ignoreSslErrors();
          LOG_DEBUG("QtTransportListenSocket: handshake errors ignored, continuing");
        }
    );
    m_server = sslServer;
  } else {
    m_server = new QTcpServer(this);
  }
#else
  if (wantsTls()) {
    m_server = new Qt5TlsTcpServer(this);
  } else {
    m_server = new QTcpServer(this);
  }
#endif
  LOG_DEBUG("QtTransportListenSocket: created (tls=%d)", wantsTls() ? 1 : 0);
}

QtTransportListenSocket::~QtTransportListenSocket()
{
  if (m_server && m_server->isListening()) {
    m_server->close();
  }
}

bool QtTransportListenSocket::wantsTls() const
{
  return m_securityLevel == SecurityLevel::Encrypted || m_securityLevel == SecurityLevel::PeerAuth;
}

bool QtTransportListenSocket::configureSslServer()
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
  auto *sslServer = qobject_cast<QSslServer *>(m_server);
  if (sslServer == nullptr) {
    return false;
  }

  const QString path = Settings::value(Settings::Security::Certificate).toString();
  if (path.isEmpty()) {
    LOG_ERR("QtTransportListenSocket: security/certificate setting is empty");
    return false;
  }

  QSslConfiguration config = sslServer->sslConfiguration();
  if (!loadPemIdentity(config, path)) {
    return false;
  }

  // Always request the peer certificate, like the raw stack's
  // SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT. Chain errors are
  // ignored (see onSslErrors); trust is decided by the fingerprint database.
  config.setPeerVerifyMode(QSslSocket::VerifyPeer);

  sslServer->setSslConfiguration(config);
  return true;
#else
  if (!wantsTls()) {
    return true;
  }

  auto *tlsServer = qobject_cast<Qt5TlsTcpServer *>(m_server);
  if (tlsServer == nullptr) {
    return false;
  }

  const QString path = Settings::value(Settings::Security::Certificate).toString();
  if (path.isEmpty()) {
    LOG_ERR("QtTransportListenSocket: security/certificate setting is empty");
    return false;
  }

  QSslConfiguration config;
  if (!loadPemIdentity(config, path)) {
    return false;
  }

  // Always request the peer certificate, like the raw stack.
  config.setPeerVerifyMode(QSslSocket::VerifyPeer);

  tlsServer->sslConfiguration = config;
  return true;
#endif
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

  LOG_DEBUG("QtTransportListenSocket: accepted connection (tls=%d)", wantsTls() ? 1 : 0);
  return std::make_unique<QtNetworkTransport>(tcpSocket, m_securityLevel);
}

ArchSocket QtTransportListenSocket::getSocket() const
{
  return nullptr;
}

bool QtTransportListenSocket::bindAndListen(const NetworkAddress &address)
{
  if (!m_server) {
    LOG_ERR("QtTransportListenSocket: no server");
    return false;
  }

  if (wantsTls() && !configureSslServer()) {
    return false;
  }

  QHostAddress addr =
      address.getHostname().empty() ? QHostAddress::Any : QHostAddress(QString::fromStdString(address.getHostname()));

  if (!m_server->listen(addr, static_cast<quint16>(address.getPort()))) {
    LOG_ERR("QtTransportListenSocket: failed to listen: %s", m_server->errorString().toStdString().c_str());
    return false;
  }

  LOG_INFO(
      "QtTransportListenSocket: listening on %s:%d (tls=%d)", address.getHostname().c_str(), address.getPort(),
      wantsTls() ? 1 : 0
  );
  return true;
}

quint16 QtTransportListenSocket::serverPort() const
{
  return m_server ? m_server->serverPort() : 0;
}

int QtTransportListenSocket::getServerSocketDescriptor() const
{
  if (m_server) {
    return m_server->socketDescriptor();
  }
  return -1;
}
