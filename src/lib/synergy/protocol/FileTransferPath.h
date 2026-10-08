/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 TuPig
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <string>
#include <string_view>
#include <vector>

//! Filename handling for files arriving over the wire.
/*!
A received file name is attacker-controlled: the peer is a trust boundary, and a
compromised or spoofed one can send anything it likes. Writing to a path built
from that string is the classic directory traversal bug, so names are filtered
down to a single, boring filename rather than being sanitised in place.

The rule is reject-if-unsure. A name that cannot be reduced to something clearly
safe yields an empty string; callers must treat that as "do not write this file"
rather than substituting a guessed name, so a hostile name can never silently
land somewhere unexpected.
*/
class FileTransferPath
{
public:
  //! Reduce a wire-supplied name to a safe file name, or empty if it cannot be.
  /*!
  Rejects: empty names, `.` and `..`, anything holding a path separator or drive
  letter (so neither `../../etc/passwd` nor `C:\Windows\x` survives), NUL and
  other control characters, Windows reserved device names, names with trailing
  dot or space, and names longer than the filesystem's usual limit.
  \param raw the name as received
  \return a safe single-segment name, or empty when the name must be refused
  */
  static std::string sanitizeFileName(std::string_view raw);

  //! Append a sanitized name to \p directory.
  /*!
  \param directory target directory, without a trailing separator
  \param safeName a name that already passed sanitizeFileName()
  \return the joined path
  */
  static std::string joinUnderDirectory(const std::string &directory, const std::string &safeName);

  //! Encode names for the `DDRG` payload: NUL-separated, as the protocol states.
  static std::string joinNames(const std::vector<std::string> &names);

  //! Split a `DDRG` payload back into names.
  /*!
  Empty segments are dropped, because a trailing separator is normal and a
  stray empty name is never something to act on.
  \param payload the NUL-separated name list
  \return the names found, in order
  */
  static std::vector<std::string> splitNames(std::string_view payload);
};
