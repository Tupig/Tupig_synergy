/*
 * Synergy -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Synergy Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "client/ClientConfig.h"
#include "common/NetworkProtocol.h"
#include "common/Settings.h"
#include "net/TlsConfig.h"
#include "synergy/protocol/FileTransferSettings.h"

//! Translate the user settings into the plain structs the core layers take.
/*!
Only the application layer reads the Settings singleton; the network, server
and client cores receive these values by constructor injection, which keeps
them constructible (and testable) without a QSettings instance.
*/
namespace synergy::core::AppSettings {

inline FileTransferSettings fileTransferSettings()
{
  FileTransferSettings settings;
  settings.enabled = Settings::value(Settings::FileTransfer::Enabled).toBool();
  settings.dropDirectory = Settings::value(Settings::FileTransfer::DropDirectory).toString().toStdString();
  settings.maxFileSizeMb = Settings::value(Settings::FileTransfer::MaxFileSizeMb).toULongLong();
  settings.maxFileCount = Settings::value(Settings::FileTransfer::MaxFileCount).toULongLong();
  return settings;
}

inline TlsConfig tlsConfig()
{
  TlsConfig tls;
  tls.certificatePath = Settings::value(Settings::Security::Certificate).toString().toStdString();
  tls.trustedClientsDbPath = Settings::tlsTrustedClientsDb().toStdString();
  tls.trustedServersDbPath = Settings::tlsTrustedServersDb().toStdString();
  return tls;
}

inline NetworkProtocol serverProtocol()
{
  return Settings::value(Settings::Server::Protocol).value<NetworkProtocol>();
}

inline ClientConfig clientConfig()
{
  ClientConfig config;
  config.useSecureNetwork = Settings::value(Settings::Security::TlsEnabled).toBool();
  config.maxClipboardReceiveBytes =
      static_cast<size_t>(Settings::value(Settings::Server::ClipboardSize).toUInt()) * 1024 * 1024;
  config.bindInterface = Settings::value(Settings::Core::Interface).toString().toStdString();

  const auto blocked = Settings::value(Settings::Security::BlockedKeyCombos).toStringList();
  config.serverProxy.blockedKeyCombos.reserve(static_cast<size_t>(blocked.size()));
  for (const auto &entry : blocked) {
    config.serverProxy.blockedKeyCombos.push_back(entry.toStdString());
  }
  config.serverProxy.blockedKeyCombosConfigured = Settings::contains(Settings::Security::BlockedKeyCombos);
  config.serverProxy.fileTransfer = fileTransferSettings();
  return config;
}

} // namespace synergy::core::AppSettings
