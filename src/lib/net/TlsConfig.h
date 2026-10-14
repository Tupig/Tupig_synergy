/*
 * Synergy -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Synergy Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <string>

//! Certificate and trust-store paths for TLS.
/*!
Collected at the application layer from the user settings and injected into
the network stack, so the network stack never reads the settings singleton
itself.
*/
struct TlsConfig
{
  //! PEM file holding the local certificate (and private key).
  std::string certificatePath;
  //! TOFU database of accepted client fingerprints (server side).
  std::string trustedClientsDbPath;
  //! TOFU database of accepted server fingerprints (client side).
  std::string trustedServersDbPath;
};
