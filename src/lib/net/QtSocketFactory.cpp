/*
 * Synergy -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 TuPig
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "net/QtSocketFactory.h"

#include "base/Event.h"
#include "base/IEventQueue.h"
#include "base/Log.h"
#include "common/Settings.h"
#include "ipc/CoreIpc.h"
#include "net/FingerprintDatabase.h"
#include "net/QtNetworkTransport.h"
#include "net/SecureUtils.h"
#include "net/SocketException.h"

#include <QFile>
#include <QSslCertificate>
#include <QSslKey>
#include <QSslSocket>
#include <QTimer>

#include <cstdlib>
#include <string>

//
// QtDataSocket
//

QtDataSocket::QtDataSocket(IEventQueue *events, SecurityLevel securityLevel)
    : IDataSocket(events),
      m_events(events),
      m_securityLevel(securityLevel)
{
  m_transport = std::make_unique<QtNetworkTransport>(securityLevel);
  wireTransport();
}

QtDataSocket::QtDataSocket(
    IEventQueue *events, SecurityLevel securityLevel, std::unique_ptr<QtNetworkTransport> transport
)
    : IDataSocket(events),
      m_events(events),
      m_securityLevel(securityLevel),
      m_transport(std::move(transport)),
      m_serverSide(true)
{
  // PlainText accepted sockets are already connected; TLS ones complete the
  // handshake asynchronously and stay unresolved until then.
  m_resolved = m_transport != nullptr && m_transport->isConnected();
  wireTransport();
}

QtDataSocket::~QtDataSocket()
{
  if (m_transport) {
    QObject::disconnect(m_transport.get(), nullptr, nullptr, nullptr);
    m_transport->close();
  }
}

bool QtDataSocket::wantsTls() const
{
  return m_securityLevel == SecurityLevel::Encrypted || m_securityLevel == SecurityLevel::PeerAuth;
}

void QtDataSocket::wireTransport()
{
  if (!m_transport) {
    return;
  }

  // The transport is owned by this adapter, so using it as the receiver
  // context keeps the connection lifetimes coupled.
  auto *context = m_transport.get();
  QObject::connect(context, &QtNetworkTransport::transportConnected, context, [this] { handleConnected(); });
  QObject::connect(context, &QtNetworkTransport::transportDisconnected, context, [this] { handleDisconnected(); });
  QObject::connect(context, &QtNetworkTransport::transportDataReady, context, [this] {
    sendEvent(synergy::EventTypes::StreamInputReady);
  });
  QObject::connect(context, &QtNetworkTransport::transportError, context, [this](const QString &message, bool fatal) {
    handleError(message, fatal);
  });
}

void QtDataSocket::connect(const NetworkAddress &address)
{
  if (!m_transport) {
    sendConnectionFailed(QStringLiteral("no transport"));
    return;
  }
  m_transport->connect(address);
}

void QtDataSocket::bind(const NetworkAddress &)
{
  LOG_WARN("QtDataSocket::bind not supported on data sockets");
}

void QtDataSocket::close()
{
  if (m_transport) {
    m_transport->close();
  }
  if (!m_disconnected) {
    m_disconnected = true;
    sendEvent(synergy::EventTypes::StreamOutputShutdown);
    sendEvent(synergy::EventTypes::SocketDisconnected);
  }
}

bool QtDataSocket::isFatal() const
{
  return m_clientFatal || (m_transport && m_transport->isFatal());
}

uint32_t QtDataSocket::read(void *buffer, uint32_t n)
{
  if (m_inputShutdown || !m_transport) {
    return 0;
  }
  return m_transport->read(buffer, n);
}

void QtDataSocket::write(const void *buffer, uint32_t n)
{
  if (!m_transport || m_outputShutdown || m_transport->isFatal()) {
    sendEvent(synergy::EventTypes::StreamOutputError);
    return;
  }
  m_transport->write(buffer, n);
  if (m_transport->isFatal()) {
    // The transport marks itself fatal when the write queue overflows.
    sendEvent(synergy::EventTypes::StreamOutputError);
  }
}

void QtDataSocket::flush()
{
  if (m_transport) {
    m_transport->flush();
  }
}

void QtDataSocket::shutdownInput()
{
  m_inputShutdown = true;
}

void QtDataSocket::shutdownOutput()
{
  m_outputShutdown = true;
  if (m_transport) {
    m_transport->flush();
    if (auto *socket = m_transport->getTcpSocket()) {
      socket->disconnectFromHost();
    }
  }
}

bool QtDataSocket::isReady() const
{
  return !m_inputShutdown && m_transport && m_transport->isReady();
}

uint32_t QtDataSocket::getSize() const
{
  return m_transport ? m_transport->getSize() : 0;
}

void QtDataSocket::handleConnected()
{
  if (!wantsTls()) {
    m_resolved = true;
    sendEvent(synergy::EventTypes::DataSocketConnected);
    return;
  }

  auto *ssl = m_transport ? qobject_cast<QSslSocket *>(m_transport->getTcpSocket()) : nullptr;
  if (ssl == nullptr) {
    handleTlsFailure(QStringLiteral("not a TLS socket"));
    return;
  }

  const bool peerAuth = m_securityLevel == SecurityLevel::PeerAuth;
  const bool mustVerify = !m_serverSide || peerAuth;

  const QSslCertificate cert = ssl->peerCertificate();
  if (cert.isNull()) {
    if (mustVerify) {
      handleTlsFailure(QStringLiteral("peer has no tls certificate"));
      return;
    }
    // Encrypted servers accept clients without inspecting them (parity with
    // the raw stack, where trust is only enforced for PeerAuth).
    LOG_WARN("QtDataSocket: accepted client without a tls certificate");
  } else {
    // Parity with SecureSocket: only RSA keys are size-constrained (the raw
    // stack's cert callback never runs chain verification; trust comes from
    // the fingerprint database).
    const auto key = cert.publicKey();
    if (key.algorithm() == QSsl::Rsa && key.length() > 0 && key.length() < 2048) {
      handleTlsFailure(QStringLiteral("RSA key too small (%1 bits, minimum 2048)").arg(key.length()));
      return;
    }

    if (mustVerify) {
      const QString dbPath = m_serverSide ? Settings::tlsTrustedClientsDb() : Settings::tlsTrustedServersDb();
      if (!verifyFingerprint(dbPath)) {
        handleTlsFailure(QStringLiteral("failed to verify peer certificate fingerprint"));
        return;
      }
    }

    LOG_INFO(
        "peer tls certificate info: %s", cert.subjectInfo(QSslCertificate::CommonName).join(u'/').toStdString().c_str()
    );
  }

  m_resolved = true;
  if (m_serverSide) {
    sendEvent(synergy::EventTypes::ClientListenerAccepted);
  } else {
    sendEvent(synergy::EventTypes::DataSocketSecureConnected);
  }
}

void QtDataSocket::handleDisconnected()
{
  if (m_disconnected) {
    return;
  }
  m_disconnected = true;

  if (m_serverSide && wantsTls() && !m_resolved) {
    // Handshake never completed; the listener is still waiting for the
    // accept result.
    sendEvent(synergy::EventTypes::ClientListenerDisconnectedOnAccept);
    return;
  }

  sendEvent(synergy::EventTypes::StreamInputShutdown);
  sendEvent(synergy::EventTypes::SocketDisconnected);
}

void QtDataSocket::handleError(const QString &message, bool fatal)
{
  LOG_ERR("QtDataSocket: %s", message.toStdString().c_str());
  if (!fatal) {
    return;
  }

  if (!m_resolved) {
    if (m_serverSide) {
      m_resolved = true;
      sendEvent(synergy::EventTypes::ClientListenerDisconnectedOnAccept);
    } else {
      sendConnectionFailed(message);
    }
    return;
  }

  sendEvent(synergy::EventTypes::StreamOutputError);
}

void QtDataSocket::handleTlsFailure(const QString &message)
{
  LOG_ERR("QtDataSocket: %s", message.toStdString().c_str());
  m_clientFatal = true;
  m_disconnected = true;
  if (m_serverSide) {
    m_resolved = true;
    sendEvent(synergy::EventTypes::ClientListenerDisconnectedOnAccept);
  } else {
    sendConnectionFailed(message);
  }
  if (m_transport) {
    m_transport->close();
  }
}

bool QtDataSocket::verifyFingerprint(const QString &databasePath)
{
  auto *ssl = m_transport ? qobject_cast<QSslSocket *>(m_transport->getTcpSocket()) : nullptr;
  if (ssl == nullptr) {
    return false;
  }

  const QSslCertificate cert = ssl->peerCertificate();
  if (cert.isNull()) {
    return false;
  }

  // Same digest the raw stack reads out of the X509 store (SHA-256 over the
  // DER encoding).
  const QByteArray sha256 = cert.digest(QCryptographicHash::Sha256);
  if (sha256.isEmpty()) {
    return false;
  }

  const auto fingerprint = synergy::formatSSLFingerprint(sha256, false);
  LOG_DEBUG("peer fingerprint: %s", qPrintable(fingerprint));
  ipcSendToClient("peerFingerprint", fingerprint);

  FingerprintDatabase db;
  db.read(databasePath);
  const bool emptyDB = db.fingerprints().empty();

  if (QFile::exists(databasePath) && emptyDB) {
    LOG_ERR("failed to open trusted fingerprints file: %s", qPrintable(databasePath));
    return false;
  }

  if (!db.isTrusted({QCryptographicHash::Sha256, sha256})) {
    LOG_WARN("fingerprint does not match trusted fingerprint");
    return false;
  }

  LOG_DEBUG("fingerprint matches trusted fingerprint");
  return true;
}

void QtDataSocket::sendEvent(synergy::EventTypes type)
{
  m_events->addEvent(Event(type, getEventTarget()));
}

void QtDataSocket::sendConnectionFailed(const QString &message)
{
  auto *info = new ConnectionFailedInfo(message.toStdString().c_str());
  m_events->addEvent(
      Event(synergy::EventTypes::DataSocketConnectionFailed, getEventTarget(), info, Event::EventFlags::DontFreeData)
  );
}

//
// QtListenSocket
//

QtListenSocket::QtListenSocket(IEventQueue *events, SecurityLevel securityLevel)
    : m_events(events),
      m_securityLevel(securityLevel)
{
  m_listen = std::make_unique<QtTransportListenSocket>(securityLevel);
}

QtListenSocket::~QtListenSocket() = default;

void QtListenSocket::bind(const NetworkAddress &address)
{
  // One event per pending connection; the consumer accepts until accept()
  // returns nullptr.
  QObject::connect(m_listen->getServer(), &QTcpServer::newConnection, m_listen.get(), [this] {
    m_pendingRetry.restart();
    sendEvent(synergy::EventTypes::ListenSocketConnecting);
  });

  if (m_listen->bindAndListen(address)) {
    return;
  }

  const QString error = m_listen->getServer()->errorString();
  const auto errorStd = error.toStdString();
  if (m_listen->getServer()->serverError() == QAbstractSocket::AddressInUseError) {
    throw SocketAddressInUseException(errorStd.c_str());
  }
  if (!error.isEmpty()) {
    throw SocketBindException(errorStd.c_str());
  }
  throw SocketException("failed to configure the TLS listener");
}

void QtListenSocket::close()
{
  m_listen.reset();
}

std::unique_ptr<IDataSocket> QtListenSocket::accept()
{
  if (!m_listen) {
    return nullptr;
  }

  auto transport = m_listen->accept();
  if (!transport) {
    // A TLS handshake may still be in flight (QSslServer announces the
    // connection early); re-announce briefly so the consumer retries.
    if (m_pendingRetry.isValid() && m_pendingRetry.elapsed() < 10000) {
      QTimer::singleShot(50, m_listen.get(), [this] { sendEvent(synergy::EventTypes::ListenSocketConnecting); });
    }
    return nullptr;
  }

  auto *qtTransport = dynamic_cast<QtNetworkTransport *>(transport.release());
  if (qtTransport == nullptr) {
    return nullptr;
  }

  return std::make_unique<QtDataSocket>(m_events, m_securityLevel, std::unique_ptr<QtNetworkTransport>(qtTransport));
}

quint16 QtListenSocket::serverPort() const
{
  return m_listen ? m_listen->serverPort() : 0;
}

void QtListenSocket::sendEvent(synergy::EventTypes type)
{
  m_events->addEvent(Event(type, getEventTarget()));
}

//
// QtSocketFactory
//

QtSocketFactory::QtSocketFactory(IEventQueue *events) : m_events(events)
{
}

std::unique_ptr<IDataSocket>
QtSocketFactory::create(IArchNetwork::AddressFamily /*family*/, SecurityLevel securityLevel) const
{
  return std::make_unique<QtDataSocket>(m_events, securityLevel);
}

std::unique_ptr<IListenSocket>
QtSocketFactory::createListen(IArchNetwork::AddressFamily /*family*/, SecurityLevel securityLevel) const
{
  return std::make_unique<QtListenSocket>(m_events, securityLevel);
}

bool shouldUseLegacyNetwork()
{
  const char *env = std::getenv("USE_LEGACY_NETWORK");
  return env != nullptr && std::string(env) == "1";
}
