/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "net/INetworkTransport.h"

#include <memory>

//! Qt-based TCP transport implementation
/*!
This class provides a network transport implementation using Qt's
QTcpSocket and QSslSocket.  It replaces the legacy poll-based
socket multiplexer with Qt's event-driven I/O.

This is a skeleton implementation for Step 2.  The full implementation
will be completed in Step 3 (QtTcpTransport) and Step 4 (QtTlsTransport).
*/
class QtNetworkTransport : public INetworkTransport
{
public:
  QtNetworkTransport();
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

private:
  SecurityLevel m_securityLevel = SecurityLevel::PlainText;
  bool m_connected = false;
  bool m_fatalError = false;
};

//! Qt-based listen socket implementation
/*!
This class provides a listen socket using Qt's QTcpServer.
*/
class QtTransportListenSocket : public ITransportListenSocket
{
public:
  QtTransportListenSocket();
  ~QtTransportListenSocket() override;

  std::unique_ptr<INetworkTransport> accept() override;
  ArchSocket getSocket() const override;

private:
  // TODO: QTcpServer will be added in Step 3
};
