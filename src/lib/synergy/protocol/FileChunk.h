/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 TuPig
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "Chunk.h"
#include "ProtocolTypes.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace deskflow {
class IStream;
}

//! Reassembly state for one file arriving as a sequence of chunks.
struct FileTransferAssemblyState
{
  //! Size the sender declared for the file currently being received.
  uint64_t expectedSize = 0;

  //! True between a start chunk and its matching end chunk.
  bool active = false;
};

//! Chunking and reassembly for the `DFTR` file transfer message.
/*!
Wire format is `DFTR%1i%s` - a one byte transfer mark followed by one
length-prefixed string (ProtocolTypes.cpp). The payload therefore travels
through ProtocolUtil's `%s`, which caps a string at PROTOCOL_MAX_STRING_LENGTH;
chunkSize() is derived from that constant rather than picked independently,
because a larger chunk is not merely slow - ProtocolUtil::readBytes() throws
BadClientException, which the message dispatchers treat as a protocol error and
drop the connection.

The declared file size travels as a decimal string, matching ClipboardChunk.
*/
class FileChunk : public Chunk
{
public:
  explicit FileChunk(size_t size);

  //! Build a start chunk carrying the declared file size.
  static FileChunk *start(uint64_t fileSize);

  //! Build a data chunk carrying file content.
  static FileChunk *data(const std::string &bytes);

  //! Build an end chunk.
  static FileChunk *end();

  //! Read one chunk, appending payload to \p dataCached.
  /*!
  \param stream stream positioned after the `DFTR` message code
  \param dataCached accumulates the file content across calls
  \param state reassembly state, updated in place
  \param maxFileSize largest file this receiver will accept
  \return what happened, per TransferState
  \throws BadClientException if a chunk exceeds the string transport ceiling, or
  the stream is malformed. Callers must treat this as a protocol error.
  */
  static TransferState
  assemble(deskflow::IStream *stream, std::string &dataCached, FileTransferAssemblyState &state, uint64_t maxFileSize);

  //! Write a chunk to the stream.
  static void send(deskflow::IStream *stream, void *chunk);

  //! Content bytes carried by one chunk.
  /*!
  Bounded by the transport, not chosen here; see the class comment.
  */
  static size_t chunkSize();
};

//! Split \p bytes into chunk-sized pieces for transmission.
/*!
Exposed separately from StreamChunker-style sending so the split can be tested
without an event queue. Every piece is non-empty; an empty input yields no
pieces at all.
*/
std::vector<std::string> splitIntoFileChunks(std::string_view bytes);
