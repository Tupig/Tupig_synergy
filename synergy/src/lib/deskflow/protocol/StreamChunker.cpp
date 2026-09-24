/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2013 - 2016 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "StreamChunker.h"

#include "ClipboardChunk.h"
#include "ProtocolTypes.h"
#include "base/Event.h"
#include "base/IEventQueue.h"
#include "base/Log.h"

//! Clipboard payload size per chunk.
/*!
Bounded by the receiver, not chosen here: ProtocolUtil::readBytes() rejects any
length-prefixed string larger than MessageSizeLimit::ClipboardChunk, and the
rejection surfaces as a BadClientException, i.e. a protocol error that drops the
connection. A chunk above that ceiling therefore does not degrade gracefully, so
the two must not drift apart. This was previously 512 KB, which is 8x the ceiling
and broke clipboard sync for any payload over 64 KB; ProtocolTypes.h's comment
meanwhile claimed 32 KB. Deriving the value from the protocol constant keeps a
single source of truth.
*/
static const size_t g_chunkSize = static_cast<size_t>(MessageSizeLimit::ClipboardChunk);

static_assert(
    g_chunkSize <= static_cast<size_t>(MessageSizeLimit::ClipboardChunk),
    "clipboard chunk size must fit the receiver's length-prefixed string limit"
);

size_t StreamChunker::chunkSize()
{
  return g_chunkSize;
}

void StreamChunker::sendClipboard(
    const std::string_view &data, size_t size, ClipboardID id, uint32_t sequence, IEventQueue *events, void *eventTarget
)
{
  // send first message (data size)
  std::string dataSize = QString::number(size).toStdString();
  ClipboardChunk *sizeMessage = ClipboardChunk::start(id, sequence, dataSize);

  events->addEvent(Event(EventTypes::ClipboardSending, eventTarget, sizeMessage));

  // send clipboard chunk with a fixed size
  size_t sentLength = 0;
  size_t chunkSize = g_chunkSize;

  while (true) {
    // make sure we don't read too much from the mock data.
    if (sentLength + chunkSize > size) {
      chunkSize = size - sentLength;
    }

    std::string chunk(data.substr(sentLength, chunkSize).data(), chunkSize);
    ClipboardChunk *dataChunk = ClipboardChunk::data(id, sequence, chunk);

    events->addEvent(Event(EventTypes::ClipboardSending, eventTarget, dataChunk));

    sentLength += chunkSize;
    if (sentLength == size) {
      break;
    }
  }

  // send last message
  ClipboardChunk *end = ClipboardChunk::end(id, sequence);

  events->addEvent(Event(EventTypes::ClipboardSending, eventTarget, end));

  LOG_DEBUG("sent clipboard size=%d", sentLength);
}
