/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 TuPig
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "FileTransferSender.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

//! Apply outbound file-transfer policy, then drive FileTransferSender.
/*!
Keeps Server free of inspect/limit details so the policy can be unit tested
without a live Server, network, or platform screen. The emitter still belongs to
the caller: Server wires it to sendDragInfo / fileChunkSending.
*/
class FileTransferOutbound
{
public:
  struct Options
  {
    //! When false, sendPaths is a no-op.
    bool enabled = true;

    //! Largest single file that may be sent.
    uint64_t maxFileSize = 64ull * 1024 * 1024;

    //! Largest number of files kept from one selection.
    size_t maxFileCount = 32;
  };

  //! Inspect \p paths under \p options and stream every accepted entry.
  /*!
  \return total content bytes streamed, or 0 when disabled / nothing accepted
  */
  static uint64_t
  sendPaths(const std::vector<std::string> &paths, const Options &options, const FileTransferSender::Emitter &emitter);
};
