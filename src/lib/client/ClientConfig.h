/*
 * Synergy -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Synergy Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "synergy/protocol/FileTransferSettings.h"

#include <cstddef>
#include <string>
#include <vector>

//! Configuration injected into \c ServerProxy at construction.
struct ServerProxyConfig
{
  //! Raw blocked key combinations (as configured by the user).
  std::vector<std::string> blockedKeyCombos;
  //! Whether the user explicitly configured the blocked combinations.
  bool blockedKeyCombosConfigured = false;
  FileTransferSettings fileTransfer;
};

//! Configuration injected into \c Client at construction.
/*!
Collected at the application layer from the user settings, so the client core
never reads the settings singleton itself.
*/
struct ClientConfig
{
  //! Whether the connection uses TLS (security/tlsEnabled).
  bool useSecureNetwork = false;
  //! Maximum size of an incoming clipboard (server/clipboardSize, in bytes).
  size_t maxClipboardReceiveBytes = 0;
  //! Network interface to bind to; empty leaves the choice to the OS.
  std::string bindInterface;
  ServerProxyConfig serverProxy;
};
