/*
 * Synergy -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
 * SPDX-FileCopyrightText: (C) 2015 - 2016 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "SecureListenSocket.h"

#include "SecureSocket.h"
#include "arch/Arch.h"
#include "arch/ArchException.h"
#include "net/SocketMultiplexer.h"

#include <QString>

//
// SecureListenSocket
//

SecureListenSocket::SecureListenSocket(
    IEventQueue *events, SocketMultiplexer *socketMultiplexer, IArchNetwork::AddressFamily family,
    SecurityLevel securityLevel, const TlsConfig &tlsConfig
)
    : TCPListenSocket(events, socketMultiplexer, family),
      m_securityLevel{securityLevel},
      m_tlsConfig(tlsConfig)

{
  // do nothing
}

std::unique_ptr<IDataSocket> SecureListenSocket::accept()
{
  std::unique_ptr<SecureSocket> secureSocket;
  try {
    secureSocket = std::make_unique<SecureSocket>(
        events(), socketMultiplexer(), ARCH->acceptSocket(socket(), nullptr), m_securityLevel, m_tlsConfig
    );
    secureSocket->initSsl(true);

    setListeningJob();

    // default location of the TLS cert file in users dir
    if (!secureSocket->loadCertificate(QString::fromStdString(m_tlsConfig.certificatePath))) {
      return nullptr;
    }

    secureSocket->secureAccept();

    return secureSocket;
  } catch (ArchNetworkException &) {
    if (secureSocket) {
      setListeningJob();
    }
    return nullptr;
  } catch (std::exception &) {
    if (secureSocket) {
      setListeningJob();
    }
    throw;
  }
}
