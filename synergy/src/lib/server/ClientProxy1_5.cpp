/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2013 - 2016 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "server/ClientProxy1_5.h"

#include "FileChunk.h"
#include "FileTransferPath.h"
#include "ProtocolUtil.h"
#include "StreamChunker.h"
#include "base/Log.h"
#include "io/IStream.h"
#include "server/Server.h"

#include <charconv>
#include <cstring>
#include <memory>

namespace {

//! Turn the decimal size payload of a start chunk back into a number.
/*!
Returns false unless the whole payload parsed, so trailing junk is rejected
rather than silently ignored.
*/
bool parseDeclaredSize(const std::string &payload, uint64_t &size)
{
  const auto *begin = payload.data();
  const auto *end = begin + payload.size();

  const auto result = std::from_chars(begin, end, size);
  return result.ec == std::errc() && result.ptr == end;
}

} // namespace

//
// ClientProxy1_5
//

ClientProxy1_5::ClientProxy1_5(const std::string &name, deskflow::IStream *stream, Server *server, IEventQueue *events)
    : ClientProxy1_4(name, stream, server, events)
{
  // do nothing
}

void ClientProxy1_5::sendDragInfo(uint32_t fileCount, const char *info, size_t size)
{
  auto *stream = getStream();
  if (stream == nullptr) {
    LOG_ERR("cannot send drag info to \"%s\": no stream", getName().c_str());
    return;
  }

  std::string names(info, size);

  // The count is a separate protocol field, so it has to agree with the payload
  // or the peer would believe in a different number of files than it can see.
  const auto actual = FileTransferPath::splitNames(names).size();
  if (static_cast<size_t>(fileCount) != actual) {
    LOG_WARN(
        "drag info for \"%s\" declared %u file(s) but carries %zu; sending %zu",
        getName().c_str(), fileCount, actual, actual
    );
    fileCount = static_cast<uint32_t>(actual);
  }

  ProtocolUtil::writef(stream, kMsgDDragInfo, fileCount, &names);
  LOG_DEBUG("sent drag info for \"%s\": %u file(s), %zu bytes", getName().c_str(), fileCount, size);
}

void ClientProxy1_5::fileChunkSending(uint8_t mark, char *data, size_t dataSize)
{
  auto *stream = getStream();
  if (stream == nullptr) {
    LOG_ERR("cannot send file chunk to \"%s\": no stream", getName().c_str());
    return;
  }

  // Build the chunk through FileChunk's factories instead of formatting the
  // message here, so the wire encoding exists in exactly one place - the one the
  // unit tests pin. A second hand-rolled encoder could drift away from the
  // decoder without anyone noticing.
  const std::string payload(data, dataSize);
  std::unique_ptr<FileChunk> chunk;

  switch (mark) {
  case ChunkType::DataStart: {
    uint64_t declaredSize = 0;
    if (!parseDeclaredSize(payload, declaredSize)) {
      LOG_ERR("refusing to send file chunk to \"%s\": bad size header \"%s\"", getName().c_str(), payload.c_str());
      return;
    }
    chunk.reset(FileChunk::start(declaredSize));
    break;
  }

  case ChunkType::DataChunk:
    chunk.reset(FileChunk::data(payload));
    break;

  case ChunkType::DataEnd:
    chunk.reset(FileChunk::end());
    break;

  default:
    LOG_ERR("unknown file chunk mark %u for \"%s\"", mark, getName().c_str());
    return;
  }

  FileChunk::send(stream, chunk.get());
  LOG_VERBOSE("sent file chunk mark=%u size=%zu to \"%s\"", mark, dataSize, getName().c_str());
}

bool ClientProxy1_5::parseMessage(const uint8_t *code)
{
  if (memcmp(code, kMsgDFileTransfer, 4) == 0) {
    fileChunkReceived();
  } else if (memcmp(code, kMsgDDragInfo, 4) == 0) {
    dragInfoReceived();
  } else {
    return ClientProxy1_4::parseMessage(code);
  }

  return true;
}

void ClientProxy1_5::fileChunkReceived() const
{
  // Receiving a transfer is the reverse direction (secondary -> primary), which
  // is not implemented. Say so rather than silently discarding the message: a
  // quiet no-op here looks identical to a transfer that worked.
  LOG_WARN("ignoring file chunk from \"%s\": receiving transfers is not implemented", getName().c_str());
}

void ClientProxy1_5::dragInfoReceived() const
{
  LOG_WARN("ignoring drag info from \"%s\": receiving transfers is not implemented", getName().c_str());
}
