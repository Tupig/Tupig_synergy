/*
 * Synergy -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "net/TCPSocketFactory.h"
#include "net/SecureListenSocket.h"
#include "net/SecureSocket.h"
#include "net/TCPListenSocket.h"
#include "net/TCPSocket.h"

//
// TCPSocketFactory
//

TCPSocketFactory::TCPSocketFactory(IEventQueue *events, SocketMultiplexer *socketMultiplexer)
    : m_events(events),
      m_socketMultiplexer(socketMultiplexer)
{
  // do nothing
}

std::unique_ptr<IDataSocket>
TCPSocketFactory::create(IArchNetwork::AddressFamily family, SecurityLevel securityLevel) const
{
  if (securityLevel != SecurityLevel::PlainText) {
    auto secureSocket = std::make_unique<SecureSocket>(m_events, m_socketMultiplexer, family, securityLevel);
    secureSocket->initSsl(false);
    return secureSocket;
  }
  return std::make_unique<TCPSocket>(m_events, m_socketMultiplexer, family);
}

std::unique_ptr<IListenSocket>
TCPSocketFactory::createListen(IArchNetwork::AddressFamily family, SecurityLevel securityLevel) const
{
  if (securityLevel != SecurityLevel::PlainText) {
    return std::make_unique<SecureListenSocket>(m_events, m_socketMultiplexer, family, securityLevel);
  }
  return std::make_unique<TCPListenSocket>(m_events, m_socketMultiplexer, family);
}
