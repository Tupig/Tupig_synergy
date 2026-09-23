/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 TuPig
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "FileTransferOutbound.h"

#include "base/Log.h"

uint64_t FileTransferOutbound::sendPaths(
    const std::vector<std::string> &paths, const Options &options, const FileTransferSender::Emitter &emitter
)
{
  if (!options.enabled) {
    LOG_DEBUG("file transfer outbound: disabled; not sending");
    return 0;
  }

  if (paths.empty()) {
    return 0;
  }

  auto entries = FileTransferSource::inspectAll(paths, options.maxFileSize);
  if (entries.empty()) {
    LOG_WARN("file transfer outbound: no readable files in the selection");
    return 0;
  }

  if (entries.size() > options.maxFileCount) {
    LOG_WARN(
        "file transfer outbound: trimming selection from %zu to %zu file(s)", entries.size(), options.maxFileCount
    );
    entries.resize(options.maxFileCount);
  }

  return FileTransferSender::send(entries, emitter);
}
