/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "net/INetworkTransport.h"

class IDataSocket;
class IListenSocket;
class IEventQueue;
class SocketMultiplexer;

//! Legacy network transport implementation
/*!
This class wraps the existing TCPSocket/SecureSocket implementations
to provide the INetworkTransport interface.  It serves as a fallback
and reference implementation for the Qt-based transport.
*/
class LegacyNetworkTransport : public INetworkTransport
{
public:
  //! Create a data transport wrapping an existing socket
  LegacyNetworkTransport(IDataSocket *socket, IEventQueue *events);

  //! Create a listen transport wrapping an existing socket
  LegacyNetworkTransport(IListenSocket *socket, IEventQueue *events);

  ~LegacyNetworkTransport() override;

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

  //! Get the underlying data socket (for accept operations)
  IDataSocket *getSocketImpl() const;

private:
  IDataSocket *m_dataSocket = nullptr;
  IListenSocket *m_listenSocket = nullptr;
  IEventQueue *m_events;
  SecurityLevel m_securityLevel = SecurityLevel::PlainText;
};

//! Legacy listen socket implementation
/*!
This class wraps IListenSocket to provide ITransportListenSocket interface.
*/
class LegacyTransportListenSocket : public ITransportListenSocket
{
public:
  LegacyTransportListenSocket(IListenSocket *socket);
  ~LegacyTransportListenSocket() override;

  std::unique_ptr<INetworkTransport> accept() override;
  ArchSocket getSocket() const override;

private:
  IListenSocket *m_socket;
};
