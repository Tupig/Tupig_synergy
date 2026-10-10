/*
 * Synergy -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 TuPig Synergy Contributors
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <QString>

namespace synergy {

//! Path of the hello token for an IPC server name (`synergy-daemon` or `synergy-core`).
QString ipcTokenPath(const QString &serverName);

//! 64 hex characters from the system random generator.
QString generateIpcToken();

//! Read a token written by the matching server. Empty when the file is missing.
QString readIpcToken(const QString &serverName);

//! Replace the token file. On Windows the DACL is applied as the file is
//! created: SYSTEM, Administrators, and the active console user. When the
//! console user cannot be resolved, the writing process is granted access
//! instead, so a desktop core can still read its own token.
bool writeIpcToken(const QString &serverName, const QString &token);

//! Re-apply the token file ACL. The daemon calls this after a logon so the
//! console user can read a token that was created before anyone had logged on.
bool refreshIpcTokenAccess(const QString &serverName);

} // namespace synergy
