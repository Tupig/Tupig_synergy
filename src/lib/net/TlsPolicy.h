/*
 * Synergy -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 TuPig
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "net/SecurityLevel.h"

//! Shortest RSA key the Qt stack and the legacy OpenSSL stack both accept.
inline constexpr int kMinimumRsaBits = 2048;

//! Encrypted and peer-auth connections both require a peer certificate.
/*!
A missing peer certificate is rejected on both stacks. The client always
checks the server fingerprint. The server checks the client fingerprint only
for \c SecurityLevel::PeerAuth.
*/
inline bool tlsRequiresPeerCertificate(SecurityLevel level)
{
  return level == SecurityLevel::Encrypted || level == SecurityLevel::PeerAuth;
}
