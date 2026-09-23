/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 TuPig
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "FileChunk.h"

#include "ProtocolUtil.h"
#include "base/Log.h"
#include "deskflow/core/DeskflowException.h"
#include "io/IStream.h"

#include <cstring>
#include <limits>
#include <stdexcept>

namespace {

//! Bytes of bookkeeping in a chunk buffer: the mark plus a terminating NUL.
constexpr size_t s_fileChunkMetaSize = 2;

//! Content ceiling for one chunk, taken from the transport.
const size_t g_chunkSize = static_cast<size_t>(PROTOCOL_MAX_STRING_LENGTH);

static_assert(
    g_chunkSize > 0 && g_chunkSize <= static_cast<size_t>(PROTOCOL_MAX_STRING_LENGTH),
    "file chunk size must fit the receiver's length-prefixed string limit"
);

void clearCachedData(std::string &dataCached)
{
  dataCached.clear();
  dataCached.shrink_to_fit();
}

bool wouldExceed(uint64_t currentSize, size_t extraSize, uint64_t limit)
{
  return currentSize > limit || extraSize > limit - static_cast<size_t>(currentSize);
}

} // namespace

size_t FileChunk::chunkSize()
{
  return g_chunkSize;
}

FileChunk::FileChunk(size_t size) : Chunk(size)
{
  m_dataSize = size - s_fileChunkMetaSize;
}

FileChunk *FileChunk::start(uint64_t fileSize)
{
  const std::string size = std::to_string(fileSize);
  const size_t sizeLength = size.size();

  auto *chunk = new FileChunk(sizeLength + s_fileChunkMetaSize);
  char *buffer = chunk->m_chunk;
  buffer[0] = static_cast<char>(ChunkType::DataStart);
  std::memcpy(&buffer[1], size.c_str(), sizeLength);
  buffer[sizeLength + s_fileChunkMetaSize - 1] = '\0';

  return chunk;
}

FileChunk *FileChunk::data(const std::string &bytes)
{
  const size_t dataSize = bytes.size();

  auto *chunk = new FileChunk(dataSize + s_fileChunkMetaSize);
  char *buffer = chunk->m_chunk;
  buffer[0] = static_cast<char>(ChunkType::DataChunk);
  std::memcpy(&buffer[1], bytes.data(), dataSize);
  buffer[dataSize + s_fileChunkMetaSize - 1] = '\0';

  return chunk;
}

FileChunk *FileChunk::end()
{
  auto *chunk = new FileChunk(s_fileChunkMetaSize);
  char *buffer = chunk->m_chunk;
  buffer[0] = static_cast<char>(ChunkType::DataEnd);
  buffer[s_fileChunkMetaSize - 1] = '\0';

  return chunk;
}

TransferState FileChunk::assemble(
    deskflow::IStream *stream, std::string &dataCached, FileTransferAssemblyState &state, uint64_t maxFileSize
)
{
  using enum TransferState;

  uint32_t mark = 0;
  std::string data;

  auto reset = [&]() {
    state = {};
    clearCachedData(dataCached);
  };

  // The `%s` reader throws BadClientException when the declared string length is
  // over the transport ceiling; that is deliberately not caught here, because
  // the dispatchers turn it into a clean disconnect. See FileChunk's comment.
  if (!ProtocolUtil::readf(stream, kMsgDFileTransfer + 4, &mark, &data)) {
    LOG_ERR("file chunk unreadable");
    reset();
    return Error;
  }

  if (mark == static_cast<uint32_t>(ChunkType::DataStart)) {
    // Refuse a second start while a transfer is in flight: silently restarting
    // would mix two files into one buffer.
    if (state.active) {
      LOG_ERR("file start chunk while a transfer is already active");
      reset();
      return Error;
    }

    bool ok = false;
    const auto declared = QString::fromStdString(data).toULongLong(&ok);
    if (!ok) {
      LOG_ERR("file transfer invalid size header: %s", data.c_str());
      reset();
      return Error;
    }

    clearCachedData(dataCached);
    state.expectedSize = declared;
    state.active = true;

    if (state.expectedSize > maxFileSize) {
      LOG_ERR(
          "file exceeds size limit, size: %llu, limit: %llu",
          static_cast<unsigned long long>(state.expectedSize),
          static_cast<unsigned long long>(maxFileSize)
      );
      reset();
      return Error;
    }

    // Reserve only a small amount. Reserving the declared size here would let a
    // peer allocate arbitrary memory by declaring a huge size without ever
    // sending the data - the declared size is a promise, not evidence. Growth
    // beyond this point is driven by bytes that actually arrive.
    constexpr size_t kInitialReserve = 64 * 1024;
    dataCached.reserve(
        state.expectedSize < kInitialReserve ? static_cast<size_t>(state.expectedSize) : kInitialReserve
    );
    LOG_DEBUG("start receiving file, expected size=%llu", static_cast<unsigned long long>(state.expectedSize));
    return Started;
  }

  if (mark == static_cast<uint32_t>(ChunkType::DataChunk)) {
    if (!state.active) {
      LOG_ERR("file data chunk before start");
      reset();
      return Error;
    }

    // The declared size is a promise from the peer; enforce it as an upper
    // bound so a lying sender cannot grow the buffer without limit.
    if (wouldExceed(dataCached.size(), data.size(), state.expectedSize)) {
      LOG_ERR(
          "file exceeds declared size, got: %zu, declared: %llu", dataCached.size() + data.size(),
          static_cast<unsigned long long>(state.expectedSize)
      );
      reset();
      return Error;
    }

    dataCached.append(data);
    return InProgress;
  }

  if (mark == static_cast<uint32_t>(ChunkType::DataEnd)) {
    if (!state.active) {
      LOG_ERR("file end chunk before start");
      reset();
      return Error;
    }

    state.active = false;

    // A short transfer means data was lost; a long one was already rejected
    // above. Either way the file is not what the sender declared.
    if (state.expectedSize != dataCached.size()) {
      LOG_ERR(
          "corrupted file data, expected size=%llu actual size=%zu",
          static_cast<unsigned long long>(state.expectedSize), dataCached.size()
      );
      reset();
      return Error;
    }

    return Finished;
  }

  LOG_ERR("unknown file chunk mark: %u", mark);
  reset();
  return Error;
}

void FileChunk::send(deskflow::IStream *stream, void *chunk)
{
  const auto *fileChunk = static_cast<FileChunk *>(chunk);
  const char *buffer = fileChunk->m_chunk;
  const auto mark = static_cast<uint32_t>(static_cast<uint8_t>(buffer[0]));
  std::string payload(&buffer[1], fileChunk->m_dataSize);

  ProtocolUtil::writef(stream, kMsgDFileTransfer, mark, &payload);
}

std::vector<std::string> splitIntoFileChunks(std::string_view bytes)
{
  std::vector<std::string> chunks;

  for (size_t offset = 0; offset < bytes.size(); offset += g_chunkSize) {
    chunks.emplace_back(bytes.substr(offset, g_chunkSize));
  }

  return chunks;
}
