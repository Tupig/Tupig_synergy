/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Chris Rizzitello <sithlord48@gmail.com>
 * SPDX-FileCopyrightText: (C) 2015 - 2016 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "ClipboardChunksTests.h"

#include "ClipboardChunk.h"
#include "ProtocolTypes.h"
#include "ProtocolUtil.h"
#include "StreamChunker.h"
#include "deskflow/core/DeskflowException.h"
#include "io/IStream.h"

#include <algorithm>
#include <cstring>
#include <deque>
#include <string>

namespace {

class MemoryStream : public deskflow::IStream
{
public:
  void push(const std::string &bytes)
  {
    m_queue.push_back(bytes);
  }

  void close() override
  {
    m_queue.clear();
    m_inputShutdown = true;
  }

  uint32_t read(void *buffer, uint32_t n) override
  {
    if (m_inputShutdown || m_queue.empty() || n == 0) {
      return 0;
    }

    auto &front = m_queue.front();
    const size_t take = std::min(static_cast<size_t>(n), front.size());
    if (buffer != nullptr) {
      std::memcpy(buffer, front.data(), take);
    }

    front.erase(0, take);
    if (front.empty()) {
      m_queue.pop_front();
    }

    return static_cast<uint32_t>(take);
  }

  void write(const void *, uint32_t) override
  {
  }

  void flush() override
  {
  }

  void shutdownInput() override
  {
    close();
  }

  void shutdownOutput() override
  {
  }

  void *getEventTarget() const override
  {
    return const_cast<MemoryStream *>(this);
  }

  bool isReady() const override
  {
    return !m_inputShutdown && !m_queue.empty();
  }

  uint32_t getSize() const override
  {
    size_t total = 0;
    for (const auto &chunk : m_queue) {
      total += chunk.size();
    }
    return static_cast<uint32_t>(std::min<size_t>(total, UINT32_MAX));
  }

private:
  std::deque<std::string> m_queue;
  bool m_inputShutdown = false;
};

class BufferWriteStream : public deskflow::IStream
{
public:
  const std::string &str() const
  {
    return m_buffer;
  }

  void close() override
  {
    m_outputShutdown = true;
  }

  uint32_t read(void *, uint32_t) override
  {
    return 0;
  }

  void write(const void *buffer, uint32_t n) override
  {
    if (!m_outputShutdown && n != 0) {
      m_buffer.append(static_cast<const char *>(buffer), n);
    }
  }

  void flush() override
  {
  }

  void shutdownInput() override
  {
  }

  void shutdownOutput() override
  {
    m_outputShutdown = true;
  }

  void *getEventTarget() const override
  {
    return const_cast<BufferWriteStream *>(this);
  }

  bool isReady() const override
  {
    return false;
  }

  uint32_t getSize() const override
  {
    return 0;
  }

private:
  std::string m_buffer;
  bool m_outputShutdown = false;
};

std::string encodeClipboardMsg(ClipboardID id, uint32_t seq, uint8_t mark, const std::string &data)
{
  BufferWriteStream stream;
  auto payload = data;
  ProtocolUtil::writef(&stream, kMsgDClipboard + 4, id, seq, mark, &payload);
  return stream.str();
}

} // namespace

void ClipboardChunksTests::initTestCase()
{
  m_log.setFilter(LogLevel::Level::Debug);
}

void ClipboardChunksTests::startFormatData()
{
  ClipboardID id = 0;
  uint32_t sequence = 0;
  std::string mockDataSize("10");
  ClipboardChunk *chunk = ClipboardChunk::start(id, sequence, mockDataSize);
  uint32_t temp_m_chunk;
  memcpy(&temp_m_chunk, &(chunk->m_chunk[1]), 4);

  QCOMPARE(chunk->m_chunk[0], id);
  QCOMPARE(temp_m_chunk, sequence);
  QCOMPARE(chunk->m_chunk[5], ChunkType::DataStart);
  QCOMPARE(chunk->m_chunk[6], '1');
  QCOMPARE(chunk->m_chunk[7], '0');
  QCOMPARE(chunk->m_chunk[8], '\0');
  delete chunk;
}

void ClipboardChunksTests::formatDataChunk()
{
  ClipboardID id = 0;
  uint32_t sequence = 1;
  uint32_t temp_m_chunk;
  std::string mockData("mock data");
  ClipboardChunk *chunk = ClipboardChunk::data(id, sequence, mockData);
  memcpy(&temp_m_chunk, &chunk->m_chunk[1], 4);

  QCOMPARE(chunk->m_chunk[0], id);
  QCOMPARE(temp_m_chunk, sequence);
  QCOMPARE(chunk->m_chunk[5], ChunkType::DataChunk);
  QCOMPARE(chunk->m_chunk[6], 'm');
  QCOMPARE(chunk->m_chunk[7], 'o');
  QCOMPARE(chunk->m_chunk[8], 'c');
  QCOMPARE(chunk->m_chunk[9], 'k');
  QCOMPARE(chunk->m_chunk[10], ' ');
  QCOMPARE(chunk->m_chunk[11], 'd');
  QCOMPARE(chunk->m_chunk[12], 'a');
  QCOMPARE(chunk->m_chunk[13], 't');
  QCOMPARE(chunk->m_chunk[14], 'a');
  QCOMPARE(chunk->m_chunk[15], '\0');

  delete chunk;
}

void ClipboardChunksTests::endFormatData()
{
  ClipboardID id = 1;
  uint32_t sequence = 1;
  uint32_t temp_m_chunk;
  ClipboardChunk *chunk = ClipboardChunk::end(id, sequence);
  memcpy(&temp_m_chunk, &chunk->m_chunk[1], 4);

  QCOMPARE(chunk->m_chunk[0], id);
  QCOMPARE(temp_m_chunk, sequence);
  QCOMPARE(chunk->m_chunk[5], ChunkType::DataEnd);
  QCOMPARE(chunk->m_chunk[6], '\0');

  delete chunk;
}

void ClipboardChunksTests::assembleAllowsDataAtExpectedSizeAndLimit()
{
  MemoryStream stream;
  stream.push(encodeClipboardMsg(0, 7, ChunkType::DataStart, "4"));
  stream.push(encodeClipboardMsg(0, 7, ChunkType::DataChunk, "AB"));
  stream.push(encodeClipboardMsg(0, 7, ChunkType::DataChunk, "CD"));
  stream.push(encodeClipboardMsg(0, 7, ChunkType::DataEnd, ""));

  std::string cached;
  ClipboardID id = kClipboardEnd;
  uint32_t seq = 0;
  ClipboardChunkAssemblyState state;

  QCOMPARE(ClipboardChunk::assemble(&stream, cached, id, seq, state, 4), TransferState::Started);
  QCOMPARE(ClipboardChunk::assemble(&stream, cached, id, seq, state, 4), TransferState::InProgress);
  QCOMPARE(ClipboardChunk::assemble(&stream, cached, id, seq, state, 4), TransferState::InProgress);
  QCOMPARE(ClipboardChunk::assemble(&stream, cached, id, seq, state, 4), TransferState::Finished);

  QCOMPARE(cached, std::string("ABCD"));
  QCOMPARE(id, static_cast<ClipboardID>(0));
  QCOMPARE(seq, static_cast<uint32_t>(7));
  QCOMPARE(ClipboardChunk::getExpectedSize(state), static_cast<size_t>(4));
  QVERIFY(!state.active);
}

void ClipboardChunksTests::assembleRejectsDataBeyondExpectedSize()
{
  MemoryStream stream;
  stream.push(encodeClipboardMsg(0, 7, ChunkType::DataStart, "1"));
  stream.push(encodeClipboardMsg(0, 7, ChunkType::DataChunk, "AA"));

  std::string cached;
  ClipboardID id = kClipboardEnd;
  uint32_t seq = 0;
  ClipboardChunkAssemblyState state;

  QCOMPARE(ClipboardChunk::assemble(&stream, cached, id, seq, state, 1024), TransferState::Started);
  QCOMPARE(ClipboardChunk::assemble(&stream, cached, id, seq, state, 1024), TransferState::Error);
  QVERIFY(cached.empty());
  QCOMPARE(ClipboardChunk::getExpectedSize(state), static_cast<size_t>(0));
  QVERIFY(!state.active);
}

void ClipboardChunksTests::assembleRejectsExpectedSizeBeyondLimit()
{
  MemoryStream stream;
  stream.push(encodeClipboardMsg(0, 7, ChunkType::DataStart, "8"));

  std::string cached;
  ClipboardID id = kClipboardEnd;
  uint32_t seq = 0;
  ClipboardChunkAssemblyState state;

  QCOMPARE(ClipboardChunk::assemble(&stream, cached, id, seq, state, 4), TransferState::Error);
  QVERIFY(cached.empty());
  QCOMPARE(ClipboardChunk::getExpectedSize(state), static_cast<size_t>(0));
  QVERIFY(!state.active);
}

void ClipboardChunksTests::assembleAcceptsChunkAtStringLengthLimit()
{
  using limits = MessageSizeLimit;
  const auto atLimit = static_cast<size_t>(limits::ClipboardChunk);

  MemoryStream stream;
  stream.push(encodeClipboardMsg(0, 7, ChunkType::DataStart, std::to_string(atLimit)));
  stream.push(encodeClipboardMsg(0, 7, ChunkType::DataChunk, std::string(atLimit, 'A')));

  std::string cached;
  ClipboardID id = kClipboardEnd;
  uint32_t seq = 0;
  ClipboardChunkAssemblyState state;

  // A payload exactly at the ceiling must still go through, or the limit would
  // be off by one for every sender that fills chunks to capacity.
  QCOMPARE(ClipboardChunk::assemble(&stream, cached, id, seq, state, atLimit * 2), TransferState::Started);
  QCOMPARE(ClipboardChunk::assemble(&stream, cached, id, seq, state, atLimit * 2), TransferState::InProgress);
  QCOMPARE(cached.size(), atLimit);
}

void ClipboardChunksTests::assembleRejectsChunkBeyondStringLengthLimit()
{
  using limits = MessageSizeLimit;
  const auto atLimit = static_cast<size_t>(limits::ClipboardChunk);
  const auto beyond = atLimit + 1;

  MemoryStream stream;
  stream.push(encodeClipboardMsg(0, 7, ChunkType::DataStart, std::to_string(beyond)));
  stream.push(encodeClipboardMsg(0, 7, ChunkType::DataChunk, std::string(beyond, 'A')));

  std::string cached;
  ClipboardID id = kClipboardEnd;
  uint32_t seq = 0;
  ClipboardChunkAssemblyState state;

  QCOMPARE(ClipboardChunk::assemble(&stream, cached, id, seq, state, beyond * 2), TransferState::Started);

  // ProtocolUtil::readBytes() throws BadClientException for a length-prefixed
  // string above MessageSizeLimit::ClipboardChunk, and readf() only converts
  // IOException/bad_alloc into a false return - so this deliberately escapes
  // assemble() rather than coming back as TransferState::Error. The contract is
  // that the caller catches BadClientException and treats it as a protocol
  // error: both ServerProxy::handleData and ClientProxy1_0::parseMessage do
  // exactly that and disconnect. Pinned here because a sender emitting larger
  // chunks depends on it (see StreamChunker).
  bool threw = false;
  try {
    ClipboardChunk::assemble(&stream, cached, id, seq, state, beyond * 2);
  } catch (const BadClientException &) {
    threw = true;
  }

  QVERIFY(threw);

  // state.active stays true: the throw escapes before assemble()'s internal
  // reset(), so no cleanup runs. That is acceptable only because every caller
  // catches BadClientException and tears the connection down; it would be a leak
  // if a caller were to swallow the exception and keep assembling.
}

void ClipboardChunksTests::sendChunkSizeFitsReceiverLimit()
{
  // Regression guard for the drift that broke large clipboards: StreamChunker
  // used to emit 512 KB chunks while the receiver refused anything over 64 KB,
  // so copying a large payload disconnected the session.
  QVERIFY(StreamChunker::chunkSize() > 0);
  QVERIFY(StreamChunker::chunkSize() <= static_cast<size_t>(MessageSizeLimit::ClipboardChunk));
}

QTEST_MAIN(ClipboardChunksTests)
