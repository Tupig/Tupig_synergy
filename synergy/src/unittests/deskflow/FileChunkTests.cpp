/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 TuPig
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "FileChunkTests.h"

#include "ProtocolTypes.h"
#include "ProtocolUtil.h"
#include "deskflow/protocol/FileChunk.h"
#include "deskflow/core/DeskflowException.h"
#include "io/IStream.h"

#include <algorithm>
#include <cstring>
#include <deque>
#include <string>

namespace {

//! Replays bytes pushed by the test, as the receiving side would read them.
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

//! Captures what a sender writes, so the exact wire bytes can be replayed.
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

//! Encode a `DFTR` message carrying mark/payload, as a sender would.
/*!
Returns the bytes *after* the 4-byte message code, because that is the stream
position FileChunk::assemble() expects: the message dispatcher consumes the code
first and only then calls assemble(), exactly as it does for ClipboardChunk.
Including the code here would feed it to the reader as payload.
*/
std::string encodeFileMsg(uint32_t mark, const std::string &payload)
{
  BufferWriteStream stream;
  auto data = payload;
  ProtocolUtil::writef(&stream, kMsgDFileTransfer, mark, &data);

  constexpr size_t kMessageCodeSize = 4;
  const auto bytes = stream.str();
  return bytes.size() < kMessageCodeSize ? std::string() : bytes.substr(kMessageCodeSize);
}

//! Encode one outbound chunk and drop its message code.
/*!
Takes ownership of \p chunk. The 4-byte code is removed because the dispatcher
consumes it once per message before calling assemble(), so the reader never sees
it - the same reason encodeFileMsg() strips it.
*/
std::string encodeChunk(FileChunk *chunk)
{
  BufferWriteStream stream;
  FileChunk::send(&stream, chunk);
  delete chunk;

  constexpr size_t kMessageCodeSize = 4;
  const auto bytes = stream.str();
  return bytes.size() < kMessageCodeSize ? std::string() : bytes.substr(kMessageCodeSize);
}

//! Send a whole file with the real sender helpers, then reassemble it.
TransferState replayThrough(FileChunk *startChunk, const std::string &content, std::string &assembled, bool includeEnd)
{
  // One queue entry per message, each already past its message code.
  std::vector<std::string> messages;
  messages.push_back(encodeChunk(startChunk));

  for (const auto &piece : splitIntoFileChunks(content)) {
    messages.push_back(encodeChunk(FileChunk::data(piece)));
  }

  if (includeEnd) {
    messages.push_back(encodeChunk(FileChunk::end()));
  }

  MemoryStream in;
  for (const auto &message : messages) {
    in.push(message);
  }

  FileTransferAssemblyState state;
  TransferState result = TransferState::Started;
  for (size_t i = 0; i < messages.size(); ++i) {
    result = FileChunk::assemble(&in, assembled, state, 64 * 1024 * 1024);
    if (result == TransferState::Error || result == TransferState::Finished) {
      break;
    }
  }

  return result;
}

} // namespace

void FileChunkTests::initTestCase()
{
  m_log.setFilter(LogLevel::Level::Verbose);
}

void FileChunkTests::chunkSizeFitsTransportLimit()
{
  // StreamChunker's clipboard bug was exactly this drift, so pin it here too:
  // a chunk above the string transport ceiling is rejected as a protocol error
  // rather than being trimmed.
  QVERIFY(FileChunk::chunkSize() > 0);
  QVERIFY(FileChunk::chunkSize() <= static_cast<size_t>(PROTOCOL_MAX_STRING_LENGTH));
}

void FileChunkTests::sendRefusesPayloadAboveTransportLimit()
{
  // A caller that builds a chunk from an arbitrary size - rather than chunking
  // through splitIntoFileChunks() - must not be able to put an over-long `%s` on
  // the wire: the receiver throws BadClientException and its dispatcher drops the
  // whole connection, so one bad file would take sharing down with it.
  const std::string oversize(FileChunk::chunkSize() + 1, 'x');

  BufferWriteStream stream;
  FileChunk *chunk = FileChunk::data(oversize);
  FileChunk::send(&stream, chunk);
  delete chunk;

  QVERIFY(stream.str().empty());

  // At the limit it must still go through, or the check would be off by one and
  // break every full-size chunk.
  const std::string atLimit(FileChunk::chunkSize(), 'y');
  BufferWriteStream okStream;
  FileChunk *ok = FileChunk::data(atLimit);
  FileChunk::send(&okStream, ok);
  delete ok;

  QVERIFY(!okStream.str().empty());
}

void FileChunkTests::splitProducesNoChunksForEmptyInput()
{
  QVERIFY(splitIntoFileChunks("").empty());
}

void FileChunkTests::splitProducesSingleChunkWhenUnderLimit()
{
  const auto pieces = splitIntoFileChunks("small");
  QCOMPARE(pieces.size(), static_cast<size_t>(1));
  QCOMPARE(pieces[0], std::string("small"));
}

void FileChunkTests::splitHonoursChunkSizeAndPreservesBytes()
{
  const size_t size = FileChunk::chunkSize() * 2 + 7;
  std::string content(size, 'x');
  for (size_t i = 0; i < size; ++i) {
    content[i] = static_cast<char>('a' + (i % 26));
  }

  const auto pieces = splitIntoFileChunks(content);
  QCOMPARE(pieces.size(), static_cast<size_t>(3));
  QCOMPARE(pieces[0].size(), FileChunk::chunkSize());
  QCOMPARE(pieces[1].size(), FileChunk::chunkSize());
  QCOMPARE(pieces[2].size(), static_cast<size_t>(7));

  std::string rejoined;
  for (const auto &piece : pieces) {
    rejoined += piece;
  }
  QCOMPARE(rejoined, content);
}

void FileChunkTests::roundTripSingleChunkFile()
{
  const std::string content = "hello file transfer";
  std::string assembled;

  QCOMPARE(
      replayThrough(FileChunk::start(content.size()), content, assembled, true), TransferState::Finished
  );
  QCOMPARE(assembled, content);
}

void FileChunkTests::roundTripFileSpanningSeveralChunks()
{
  const std::string content(FileChunk::chunkSize() + 1024, 'Z');
  std::string assembled;

  QCOMPARE(
      replayThrough(FileChunk::start(content.size()), content, assembled, true), TransferState::Finished
  );
  QCOMPARE(assembled.size(), content.size());
  QCOMPARE(assembled, content);
}

void FileChunkTests::roundTripEmptyFile()
{
  // A zero-byte file is legitimate: start declares 0, no data chunks follow.
  std::string assembled;
  QCOMPARE(replayThrough(FileChunk::start(0), "", assembled, true), TransferState::Finished);
  QVERIFY(assembled.empty());
}

void FileChunkTests::assembleRejectsDataChunkBeforeStart()
{
  MemoryStream stream;
  stream.push(encodeFileMsg(static_cast<uint32_t>(ChunkType::DataChunk), "data"));

  std::string assembled;
  FileTransferAssemblyState state;

  QCOMPARE(
      FileChunk::assemble(&stream, assembled, state, 1024), TransferState::Error
  );
  QVERIFY(assembled.empty());
  QVERIFY(!state.active);
}

void FileChunkTests::assembleRejectsEndBeforeStart()
{
  MemoryStream stream;
  stream.push(encodeFileMsg(static_cast<uint32_t>(ChunkType::DataEnd), ""));

  std::string assembled;
  FileTransferAssemblyState state;

  QCOMPARE(FileChunk::assemble(&stream, assembled, state, 1024), TransferState::Error);
  QVERIFY(!state.active);
}

void FileChunkTests::assembleRejectsSizeBeyondLimit()
{
  MemoryStream stream;
  stream.push(encodeFileMsg(static_cast<uint32_t>(ChunkType::DataStart), "2048"));

  std::string assembled;
  FileTransferAssemblyState state;

  // Declared size above the configured ceiling must be refused before any
  // content is accepted.
  QCOMPARE(FileChunk::assemble(&stream, assembled, state, 1024), TransferState::Error);
  QVERIFY(assembled.empty());
  QVERIFY(!state.active);
  QCOMPARE(state.expectedSize, static_cast<uint64_t>(0));
}

void FileChunkTests::assembleRejectsDataBeyondDeclaredSize()
{
  MemoryStream stream;
  stream.push(encodeFileMsg(static_cast<uint32_t>(ChunkType::DataStart), "2"));
  stream.push(encodeFileMsg(static_cast<uint32_t>(ChunkType::DataChunk), "AAAA"));

  std::string assembled;
  FileTransferAssemblyState state;

  QCOMPARE(FileChunk::assemble(&stream, assembled, state, 1024), TransferState::Started);
  QCOMPARE(FileChunk::assemble(&stream, assembled, state, 1024), TransferState::Error);
  QVERIFY(assembled.empty());
  QVERIFY(!state.active);
}

void FileChunkTests::assembleRejectsShortTransfer()
{
  MemoryStream stream;
  stream.push(encodeFileMsg(static_cast<uint32_t>(ChunkType::DataStart), "10"));
  stream.push(encodeFileMsg(static_cast<uint32_t>(ChunkType::DataChunk), "abc"));
  stream.push(encodeFileMsg(static_cast<uint32_t>(ChunkType::DataEnd), ""));

  std::string assembled;
  FileTransferAssemblyState state;

  QCOMPARE(FileChunk::assemble(&stream, assembled, state, 1024), TransferState::Started);
  QCOMPARE(FileChunk::assemble(&stream, assembled, state, 1024), TransferState::InProgress);
  // Ending early means data was lost; the file is not what was declared.
  QCOMPARE(FileChunk::assemble(&stream, assembled, state, 1024), TransferState::Error);
  QVERIFY(assembled.empty());
  QVERIFY(!state.active);
}

void FileChunkTests::assembleRejectsRestartWhileActive()
{
  MemoryStream stream;
  stream.push(encodeFileMsg(static_cast<uint32_t>(ChunkType::DataStart), "10"));
  stream.push(encodeFileMsg(static_cast<uint32_t>(ChunkType::DataStart), "5"));

  std::string assembled;
  FileTransferAssemblyState state;

  QCOMPARE(FileChunk::assemble(&stream, assembled, state, 1024), TransferState::Started);
  // Restarting mid-transfer would splice two files together.
  QCOMPARE(FileChunk::assemble(&stream, assembled, state, 1024), TransferState::Error);
  QVERIFY(!state.active);
}

void FileChunkTests::assembleRejectsUnknownMark()
{
  MemoryStream stream;
  stream.push(encodeFileMsg(99, "x"));

  std::string assembled;
  FileTransferAssemblyState state;

  QCOMPARE(FileChunk::assemble(&stream, assembled, state, 1024), TransferState::Error);
  QVERIFY(!state.active);
}

void FileChunkTests::assembleDoesNotPreallocateDeclaredSize()
{
  // A peer can declare a size far larger than it ever sends. Reserving the
  // declared size up front would let it demand that much memory for free, so
  // the buffer must stay near what has actually arrived.
  MemoryStream stream;
  stream.push(encodeFileMsg(static_cast<uint32_t>(ChunkType::DataStart), std::to_string(8 * 1024 * 1024)));
  stream.push(encodeFileMsg(static_cast<uint32_t>(ChunkType::DataChunk), "tiny"));

  std::string assembled;
  FileTransferAssemblyState state;

  QCOMPARE(
      FileChunk::assemble(&stream, assembled, state, 64 * 1024 * 1024), TransferState::Started
  );

  // Nothing has arrived yet beyond a declaration, so capacity must be modest.
  QVERIFY(assembled.capacity() < 1024 * 1024);

  QCOMPARE(
      FileChunk::assemble(&stream, assembled, state, 64 * 1024 * 1024), TransferState::InProgress
  );
  QCOMPARE(assembled.size(), static_cast<size_t>(4));
}

QTEST_MAIN(FileChunkTests)
