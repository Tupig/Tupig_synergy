/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
 * SPDX-FileCopyrightText: (C) 2026 TuPig
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "net/INetworkTransport.h"

#include <QByteArray>
#include <QSslSocket>
#include <QTcpServer>
#include <QTcpSocket>

#include <memory>

//! Qt-based TCP / TLS transport
/*!
Plaintext uses `QTcpSocket`. `SecurityLevel::Encrypted` and `PeerAuth` use
`QSslSocket` (Phase 2 Step 4). Certificate PEM path comes from
`Settings::Security::Certificate`, matching the legacy `SecureSocket` path.
Fingerprint / trust-store checks remain the application's responsibility for
`Encrypted`; `PeerAuth` asks Qt to verify the peer certificate.
*/
class QtNetworkTransport : public QObject, public INetworkTransport
{
  Q_OBJECT

public:
  explicit QtNetworkTransport(SecurityLevel securityLevel = SecurityLevel::PlainText);

  //! Wrap an already-connected socket (plain or SSL); takes ownership.
  QtNetworkTransport(QTcpSocket *socket, SecurityLevel securityLevel = SecurityLevel::PlainText);

  ~QtNetworkTransport() override;

  void connect(const NetworkAddress &address) override;
  void bind(const NetworkAddress &address) override;
  void close() override;
  bool isConnected() const override;

  uint32_t read(void *buffer, uint32_t size) override;
  void write(const void *buffer, uint32_t size) override;
  void flush() override;

  bool isReady() const override;
  bool isFatal() const override;
  uint32_t getSize() const override;
  ArchSocket getSocket() const override;

  void setSecurityLevel(SecurityLevel level) override;
  SecurityLevel getSecurityLevel() const override;

  QTcpSocket *getTcpSocket() const;

  void setWriteQueueLimit(size_t limit);
  size_t getWriteQueueSize() const;

  void onConnected();
  void onDisconnected();
  void onReadyRead();
  void onBytesWritten(qint64 bytes);
  void onErrorOccurred(QAbstractSocket::SocketError socketError);
  void onSslErrors(const QList<QSslError> &errors);

private:
  void createSocket();
  void connectSignals();
  void processWriteQueue();
  bool applyCertificate(QSslSocket *ssl, bool isServer);
  bool wantsTls() const;

  QTcpSocket *m_socket = nullptr;
  QByteArray m_readBuffer;
  QByteArray m_writeBuffer;
  size_t m_writeQueueLimit = 10 * 1024 * 1024;
  bool m_connected = false;
  bool m_fatalError = false;
  SecurityLevel m_securityLevel = SecurityLevel::PlainText;
};

//! Qt listen socket: `QTcpServer` or `QSslServer` depending on security level.
class QtTransportListenSocket : public QObject, public ITransportListenSocket
{
  Q_OBJECT

public:
  explicit QtTransportListenSocket(SecurityLevel securityLevel = SecurityLevel::PlainText);
  ~QtTransportListenSocket() override;

  std::unique_ptr<INetworkTransport> accept() override;
  ArchSocket getSocket() const override;

  void bindAndListen(const NetworkAddress &address);

  int getServerSocketDescriptor() const;

private:
  bool wantsTls() const;
  bool configureSslServer();

  SecurityLevel m_securityLevel = SecurityLevel::PlainText;
  QTcpServer *m_server = nullptr;
};
