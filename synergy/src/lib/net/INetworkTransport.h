/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "arch/IArchNetwork.h"
#include "net/NetworkAddress.h"
#include "net/SecurityLevel.h"

#include <cstdint>
#include <memory>

//! Network transport interface
/*!
This interface defines a unified abstraction for network transport implementations.
It provides a common API for both legacy (raw sockets) and Qt-based transport,
enabling runtime switching between implementations.

Ownership Model:
- INetworkTransport instances own their underlying socket resources
- The socket is created/owned by the factory or the accept() method
- Closing an INetworkTransport closes the underlying socket
- Destruction of INetworkTransport closes and releases the socket
*/
class INetworkTransport
{
public:
  virtual ~INetworkTransport() = default;

  //! @name connection management
  //@{

  //! Connect to a remote endpoint
  virtual void connect(const NetworkAddress &address) = 0;

  //! Bind to a local address (for server sockets)
  virtual void bind(const NetworkAddress &address) = 0;

  //! Close the connection
  virtual void close() = 0;

  //! Check if connected
  virtual bool isConnected() const = 0;

  //@}

  //! @name data transfer
  //@{

  //! Read data from the socket
  /*!
  Returns the number of bytes read, or 0 if no data available.
  */
  virtual uint32_t read(void *buffer, uint32_t size) = 0;

  //! Write data to the socket
  virtual void write(const void *buffer, uint32_t size) = 0;

  //! Flush pending writes
  virtual void flush() = 0;

  //@}

  //! @name status
  //@{

  //! Check if the socket is ready for I/O
  virtual bool isReady() const = 0;

  //! Check if the error is fatal (requires reconnection)
  virtual bool isFatal() const = 0;

  //! Get the underlying socket size (for poll operations)
  virtual uint32_t getSize() const = 0;

  //! Get the socket file descriptor (for poll operations)
  virtual ArchSocket getSocket() const = 0;

  //@}

  //! @name security
  //@{

  //! Set the security level
  virtual void setSecurityLevel(SecurityLevel level) = 0;

  //! Get the current security level
  virtual SecurityLevel getSecurityLevel() const = 0;

  //@}
};

//! Listen socket interface for transport layer
/*!
This interface defines a listen socket that accepts connections
and returns INetworkTransport instances.

Ownership Model:
- The listen socket owns the underlying socket resource
- accept() returns a new INetworkTransport with ownership transferred
- The caller owns the returned INetworkTransport instance
*/
class ITransportListenSocket
{
public:
  virtual ~ITransportListenSocket() = default;

  //! Accept an incoming connection
  /*!
  Returns a connected data socket, or nullptr if no connection is waiting.
  */
  virtual std::unique_ptr<INetworkTransport> accept() = 0;

  //! Get the socket file descriptor
  virtual ArchSocket getSocket() const = 0;
};
