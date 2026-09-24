/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2004 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "arch/IArchNetwork.h"
#include "io/IStream.h"
#include "net/ISocket.h"

#include <string>

//! Data stream socket interface
/*!
This interface defines the methods common to all network sockets that
represent a full-duplex data stream.
*/
class IDataSocket : public ISocket, public deskflow::IStream
{
public:
  class ConnectionFailedInfo
  {
  public:
    explicit ConnectionFailedInfo(const char *what) : m_what(what)
    {
      // do nothing
    }
    std::string m_what;
  };

  explicit IDataSocket([[maybe_unused]] const IEventQueue *events)
  {
    // do nothing
  }

  //! @name manipulators
  //@{

  //! Connect socket
  /*!
  Attempt to connect to a remote endpoint.  This returns immediately
  and sends a connected event when successful or a connection failed
  event when it fails.  The stream acts as if shutdown for input and
  output until the stream connects.
  */
  virtual void connect(const NetworkAddress &) = 0;

  //@}

  // ISocket overrides
  void close() override;
  void *getEventTarget() const override;

  virtual bool isFatal() const = 0;

  //! Get the underlying socket file descriptor
  /*!
  Returns the opaque socket handle for poll operations.
  This is used by the transport layer to integrate with
  platform-specific I/O multiplexing.
  */
  virtual ArchSocket getSocket() const = 0;
};
