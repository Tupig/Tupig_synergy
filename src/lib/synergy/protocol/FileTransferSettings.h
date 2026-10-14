/*
 * Synergy -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Synergy Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <cstdint>
#include <string>

//! File transfer knobs collected from the user settings.
/*!
The application layer fills this from the settings file and injects it into
the server and the client's server proxy, so the protocol layer never reads
the settings singleton itself.
*/
struct FileTransferSettings
{
  bool enabled = false;
  //! Where incoming files are written (receiver only; empty refuses them).
  std::string dropDirectory;
  //! Maximum single file size in megabytes; 0 keeps the library default.
  uint64_t maxFileSizeMb = 0;
  //! Maximum number of files per drag; 0 keeps the library default.
  uint64_t maxFileCount = 0;
};
