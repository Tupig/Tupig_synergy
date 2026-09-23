/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 TuPig
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "FileTransferSender.h"

#include "ProtocolTypes.h"
#include "base/Log.h"

uint64_t FileTransferSender::send(const std::vector<FileTransferSource::Entry> &entries, const Emitter &emitter)
{
  // An empty drag would promise the peer zero files and then send nothing.
  if (entries.empty()) {
    return 0;
  }

  if (!emitter.dragInfo || !emitter.chunk) {
    LOG_ERR("cannot send a file transfer without both emitter callbacks");
    return 0;
  }

  if (entries.size() > 0xFFFF) {
    LOG_ERR("cannot send a file transfer: %zu files exceeds the 16-bit count field", entries.size());
    return 0;
  }

  emitter.dragInfo(
      static_cast<uint32_t>(entries.size()), FileTransferSource::dragInfoPayload(entries)
  );

  uint64_t total = 0;
  FileTransferSource source;

  for (auto entry : entries) {
    // open() refreshes the size from the handle, so the declared size is the one
    // the bytes actually come from. A file edited since inspect() would otherwise
    // be announced with a stale size and the peer would reject the whole transfer.
    if (source.open(entry) != FileTransferSource::Status::Ok) {
      LOG_WARN("skipping \"%s\": it could not be opened for sending", entry.path.c_str());
      continue;
    }

    // Every started file must be closed with an end chunk, or the peer is left
    // mid-transfer and refuses the next start as a restart while active.
    emitter.chunk(static_cast<uint8_t>(ChunkType::DataStart), std::to_string(entry.size));

    for (auto piece = source.readChunk(); !piece.empty(); piece = source.readChunk()) {
      emitter.chunk(static_cast<uint8_t>(ChunkType::DataChunk), piece);
      total += piece.size();
    }

    emitter.chunk(static_cast<uint8_t>(ChunkType::DataEnd), std::string());
    source.close();
  }

  LOG_DEBUG(
      "sent %zu file(s), %llu byte(s) of content", entries.size(), static_cast<unsigned long long>(total)
  );
  return total;
}
