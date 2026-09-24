/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "net/NetworkTransportFactory.h"

#include "base/Log.h"
#include "net/IDataSocket.h"
#include "net/IListenSocket.h"
#include "net/ISocketFactory.h"
#include "net/LegacyNetworkTransport.h"
#include "net/QtNetworkTransport.h"

#include <cstdlib>

//
// NetworkTransportFactory
//

NetworkTransportFactory::NetworkTransportFactory(TransportType type, ISocketFactory *legacyFactory, IEventQueue *events)
    : m_type(type),
      m_legacyFactory(legacyFactory),
      m_events(events)
{
  // Check environment variable for override
  if (const char *env = std::getenv("USE_LEGACY_NETWORK")) {
    if (std::string(env) == "1") {
      m_type = TransportType::Legacy;
      LOG_INFO("NetworkTransportFactory: forced Legacy mode via USE_LEGACY_NETWORK=1");
    } else if (std::string(env) == "0") {
      if (isQtTransportAvailable()) {
        m_type = TransportType::Qt;
        LOG_INFO("NetworkTransportFactory: forced Qt mode via USE_LEGACY_NETWORK=0");
      } else {
        LOG_WARN("NetworkTransportFactory: Qt transport not available, using Legacy");
      }
    }
  }
}

std::unique_ptr<INetworkTransport>
NetworkTransportFactory::createTransport(IArchNetwork::AddressFamily family, SecurityLevel securityLevel)
{
  if (m_type == TransportType::Qt && isQtTransportAvailable()) {
    LOG_DEBUG("NetworkTransportFactory: creating Qt transport (security=%d)", static_cast<int>(securityLevel));
    return std::make_unique<QtNetworkTransport>(securityLevel);
  }

  // Fallback to legacy
  if (m_legacyFactory) {
    LOG_DEBUG("NetworkTransportFactory: creating Legacy transport");
    auto *socket = m_legacyFactory->create(family, securityLevel);
    return std::make_unique<LegacyNetworkTransport>(std::unique_ptr<IDataSocket>(socket), m_events);
  }

  LOG_ERR("NetworkTransportFactory: no factory available");
  return nullptr;
}

std::unique_ptr<ITransportListenSocket>
NetworkTransportFactory::createListenTransport(IArchNetwork::AddressFamily family, SecurityLevel securityLevel)
{
  if (m_type == TransportType::Qt && isQtTransportAvailable()) {
    LOG_DEBUG("NetworkTransportFactory: creating Qt listen transport (security=%d)", static_cast<int>(securityLevel));
    return std::make_unique<QtTransportListenSocket>(securityLevel);
  }

  // Fallback to legacy
  if (m_legacyFactory) {
    LOG_DEBUG("NetworkTransportFactory: creating Legacy listen transport");
    auto *socket = m_legacyFactory->createListen(family, securityLevel);
    return std::make_unique<LegacyTransportListenSocket>(std::unique_ptr<IListenSocket>(socket));
  }

  LOG_ERR("NetworkTransportFactory: no factory available");
  return nullptr;
}

TransportType NetworkTransportFactory::getTransportType() const
{
  return m_type;
}

bool NetworkTransportFactory::isQtTransportAvailable()
{
  // Qt Network 模块在构建时已链接（CMakeLists.txt 中 find_package(Network)）
  // 运行时始终可用
  return true;
}
