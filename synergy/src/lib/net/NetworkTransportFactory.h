/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "net/INetworkTransport.h"

#include <memory>

class ISocketFactory;
class IEventQueue;

//! Transport implementation type
enum class TransportType
{
  Legacy, ///< Use legacy raw socket implementation
  Qt      ///< Use Qt-based implementation
};

//! Network transport factory
/*!
This factory creates network transport instances based on the configured
implementation type.  It supports runtime switching between Legacy and
Qt implementations via the USE_LEGACY_NETWORK environment variable.
*/
class NetworkTransportFactory
{
public:
  //! Create a transport factory
  /*!
  \param type The transport type to use
  \param legacyFactory The legacy socket factory (used when type == Legacy)
  \param events The event queue for socket events
  */
  NetworkTransportFactory(
      TransportType type = TransportType::Legacy,
      ISocketFactory *legacyFactory = nullptr,
      IEventQueue *events = nullptr
  );

  ~NetworkTransportFactory() = default;

  //! Create a data transport
  std::unique_ptr<INetworkTransport> createTransport(
      IArchNetwork::AddressFamily family = IArchNetwork::AddressFamily::INet,
      SecurityLevel securityLevel = SecurityLevel::PlainText
  );

  //! Create a listen transport
  std::unique_ptr<ITransportListenSocket> createListenTransport(
      IArchNetwork::AddressFamily family = IArchNetwork::AddressFamily::INet,
      SecurityLevel securityLevel = SecurityLevel::PlainText
  );

  //! Get the configured transport type
  TransportType getTransportType() const;

  //! Check if Qt transport is available
  static bool isQtTransportAvailable();

private:
  TransportType m_type;
  ISocketFactory *m_legacyFactory;
  IEventQueue *m_events;
};
