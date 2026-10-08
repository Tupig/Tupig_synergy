/*
 * Synergy -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 TuPig
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "base/EventTypes.h"
#include "net/IDataSocket.h"
#include "net/IListenSocket.h"
#include "net/ISocketFactory.h"
#include "net/SecurityLevel.h"

#include <QElapsedTimer>
#include <QtGlobal>
#include <memory>

class QString;
class IEventQueue;
class QtNetworkTransport;
class QtTransportListenSocket;

//! Qt-backed \c IDataSocket
/*!
Bridges \c QtNetworkTransport (KTcpSocket/QSslSocket, no OS handles) into the
event-driven \c IDataSocket contract the client/server code consumes: Qt
signals become \c IEventQueue events with this socket as the target, and the
stream reads from the transport's own buffer.

TLS trust follows the raw stack: the client always verifies the server
fingerprint against the trusted-servers database, the server verifies clients
only for \c SecurityLevel::PeerAuth, and RSA keys below 2048 bits are
rejected.  The fingerprint is also reported to the GUI via
\c ipcSendToClient("peerFingerprint", ...).
*/
class QtDataSocket : public IDataSocket
{
public:
  //! Outbound (client) socket.
  QtDataSocket(IEventQueue *events, SecurityLevel securityLevel);
  //! Accepted (server-side) socket; adopts \p transport.
  QtDataSocket(IEventQueue *events, SecurityLevel securityLevel, std::unique_ptr<QtNetworkTransport> transport);
  ~QtDataSocket() override;

  // ISocket / IDataSocket
  void connect(const NetworkAddress &address) override;
  void bind(const NetworkAddress &) override;
  void close() override;
  void *getEventTarget() const override
  {
    return const_cast<QtDataSocket *>(this);
  }
  bool isFatal() const override;
  ArchSocket getSocket() const override
  {
    return nullptr;
  }

  // IStream
  uint32_t read(void *buffer, uint32_t n) override;
  void write(const void *buffer, uint32_t n) override;
  void flush() override;
  void shutdownInput() override;
  void shutdownOutput() override;
  bool isReady() const override;
  uint32_t getSize() const override;

private:
  void wireTransport();
  void handleConnected();
  void handleDisconnected();
  void handleError(const QString &message, bool fatal);
  void handleTlsFailure(const QString &message);
  bool verifyFingerprint(const QString &databasePath);
  void sendEvent(synergy::EventTypes type);
  void sendConnectionFailed(const QString &message);
  bool wantsTls() const;

  IEventQueue *m_events = nullptr;
  SecurityLevel m_securityLevel = SecurityLevel::PlainText;
  std::unique_ptr<QtNetworkTransport> m_transport;
  bool m_serverSide = false;
  bool m_inputShutdown = false;
  bool m_outputShutdown = false;
  bool m_disconnected = false;
  bool m_resolved = false;
  bool m_clientFatal = false;
};

//! Qt-backed \c IListenSocket
/*!
Wraps \c QtTransportListenSocket; forwards "connection pending" through the
\c ListenSocketConnecting event and hands accepted transports to
\c QtDataSocket.
*/
class QtListenSocket : public IListenSocket
{
public:
  QtListenSocket(IEventQueue *events, SecurityLevel securityLevel);
  ~QtListenSocket() override;

  void bind(const NetworkAddress &address) override;
  void close() override;
  void *getEventTarget() const override
  {
    return const_cast<QtListenSocket *>(this);
  }

  std::unique_ptr<IDataSocket> accept() override;

  //! Port actually bound (useful with port 0 in tests).
  quint16 serverPort() const;

private:
  void sendEvent(synergy::EventTypes type);

  IEventQueue *m_events = nullptr;
  SecurityLevel m_securityLevel = SecurityLevel::PlainText;
  std::unique_ptr<QtTransportListenSocket> m_listen;
  // QSslServer announces a connection before the TLS handshake finishes
  // and never re-announces it; retry accept() within this window.
  QElapsedTimer m_pendingRetry;
};

//! Qt socket factory
/*!
Drop-in \c ISocketFactory replacement for \c TCPSocketFactory.  The whole app
uses this by default; set `USE_LEGACY_NETWORK=1` to fall back to the raw
socket stack for one release cycle.
*/
class QtSocketFactory : public ISocketFactory
{
public:
  explicit QtSocketFactory(IEventQueue *events);

  std::unique_ptr<IDataSocket> create(
      IArchNetwork::AddressFamily family = IArchNetwork::AddressFamily::INet,
      SecurityLevel securityLevel = SecurityLevel::PlainText
  ) const override;
  std::unique_ptr<IListenSocket> createListen(
      IArchNetwork::AddressFamily family = IArchNetwork::AddressFamily::INet,
      SecurityLevel securityLevel = SecurityLevel::PlainText
  ) const override;

private:
  IEventQueue *m_events = nullptr;
};

//! True when `USE_LEGACY_NETWORK=1` selects the raw-socket stack (rollback).
bool shouldUseLegacyNetwork();
