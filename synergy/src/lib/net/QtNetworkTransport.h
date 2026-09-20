/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "net/INetworkTransport.h"

#include <QByteArray>
#include <QTcpServer>
#include <QTcpSocket>

#include <memory>

//! Qt-based TCP transport implementation
/*!
This class provides a network transport implementation using Qt's
QTcpSocket.  It replaces the legacy poll-based socket multiplexer
with Qt's event-driven I/O.

Key features:
- Non-blocking I/O via Qt event loop
- Dynamic buffer growth (no 4MB stack limit)
- Backpressure via write queue limit
- Automatic error handling via Qt signals
*/
class QtNetworkTransport : public QObject, public INetworkTransport
{
  Q_OBJECT

public:
  //! 创建客户端传输（自建 QTcpSocket）
  QtNetworkTransport();

  //! 包装已有连接的 QTcpSocket（接管所有权）
  explicit QtNetworkTransport(QTcpSocket *socket);

  ~QtNetworkTransport() override;

  // INetworkTransport overrides
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

  //! Get the underlying QTcpSocket (for Qt integration)
  QTcpSocket *getTcpSocket() const;

  //! Set write queue limit (bytes)
  void setWriteQueueLimit(size_t limit);

  //! Get current write queue size
  size_t getWriteQueueSize() const;

  // Qt signal handlers (called by connection setup)
  void onConnected();
  void onDisconnected();
  void onReadyRead();
  void onBytesWritten(qint64 bytes);
  void onErrorOccurred(QAbstractSocket::SocketError socketError);

private:
  //! 连接所有 Qt 信号到处理函数
  void connectSignals();

  //! 处理写队列
  void processWriteQueue();

  QTcpSocket *m_socket = nullptr;
  QByteArray m_readBuffer;
  QByteArray m_writeBuffer;
  size_t m_writeQueueLimit = 10 * 1024 * 1024; // 10MB default
  bool m_connected = false;
  bool m_fatalError = false;
  SecurityLevel m_securityLevel = SecurityLevel::PlainText;
};

//! Qt-based listen socket implementation
/*!
This class provides a listen socket using Qt's QTcpServer.
*/
class QtTransportListenSocket : public QObject, public ITransportListenSocket
{
  Q_OBJECT

public:
  QtTransportListenSocket();
  ~QtTransportListenSocket() override;

  std::unique_ptr<INetworkTransport> accept() override;
  ArchSocket getSocket() const override;

  //! Bind and start listening
  void bindAndListen(const NetworkAddress &address);

  //! Get the underlying QTcpServer socket descriptor
  int getServerSocketDescriptor() const;

private:
  QTcpServer *m_server = nullptr;
};
