/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 TuPig
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "FileTransferSenderTests.h"

#include "ProtocolTypes.h"
#include "ProtocolUtil.h"
#include "deskflow/protocol/FileChunk.h"
#include "deskflow/protocol/FileTransferPath.h"
#include "deskflow/protocol/FileTransferSender.h"
#include "deskflow/protocol/FileTransferSource.h"
#include "io/IStream.h"

#include <QDir>
#include <QFile>

#include <algorithm>
#include <cstring>
#include <deque>
#include <string>
#include <vector>

using Source = FileTransferSource;
using Status = FileTransferSource::Status;

namespace {

//! One message the driver produced, in order.
/*!
Named SentMessage rather than Event to avoid colliding with the global `Event`
from base/Event.h, which reaches this file through FileChunk.h -> Chunk.h.
*/
struct SentMessage
{
  enum class Kind
  {
    DragInfo,
    Chunk
  };

  Kind kind = Kind::Chunk;

  // DragInfo only.
  uint32_t fileCount = 0;
  std::string names;

  // Chunk only.
  uint8_t mark = 0;
  std::string payload;
};

//! Replays bytes as a receiver would read them.
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

//! Collects what a sender emits.
class RecordingEmitter
{
public:
  FileTransferSender::Emitter emitter()
  {
    return {
        [this](uint32_t count, const std::string &names) {
          SentMessage e;
          e.kind = SentMessage::Kind::DragInfo;
          e.fileCount = count;
          e.names = names;
          m_events.push_back(e);
        },
        [this](uint8_t mark, const std::string &payload) {
          SentMessage e;
          e.kind = SentMessage::Kind::Chunk;
          e.mark = mark;
          e.payload = payload;
          m_events.push_back(e);
        }
    };
  }

  const std::vector<SentMessage> &events() const
  {
    return m_events;
  }

  std::vector<SentMessage> chunks() const
  {
    std::vector<SentMessage> chunks;
    std::copy_if(m_events.begin(), m_events.end(), std::back_inserter(chunks), [](const SentMessage &e) {
      return e.kind == SentMessage::Kind::Chunk;
    });
    return chunks;
  }

  size_t countOf(uint8_t mark) const
  {
    return static_cast<size_t>(std::count_if(m_events.begin(), m_events.end(), [mark](const SentMessage &e) {
      return e.kind == SentMessage::Kind::Chunk && e.mark == mark;
    }));
  }

private:
  std::vector<SentMessage> m_events;
};

//! Encode a chunk the way the dispatcher's stream looks, minus the message code.
/*!
The receiver never sees the 4-byte message code: the dispatcher consumes it first
and only then calls FileChunk::assemble, exactly as it does for clipboard chunks.
Reproducing that here is what makes the round-trip faithful.
*/
std::string encodeChunkForReceiver(uint8_t mark, const std::string &payload)
{
  struct BufferStream : deskflow::IStream
  {
    std::string buffer;
    void close() override
    {
    }
    uint32_t read(void *, uint32_t) override
    {
      return 0;
    }
    void write(const void *data, uint32_t n) override
    {
      buffer.append(static_cast<const char *>(data), n);
    }
    void flush() override
    {
    }
    void shutdownInput() override
    {
    }
    void shutdownOutput() override
    {
    }
    void *getEventTarget() const override
    {
      return const_cast<BufferStream *>(this);
    }
    bool isReady() const override
    {
      return false;
    }
    uint32_t getSize() const override
    {
      return 0;
    }
  };

  BufferStream stream;
  auto data = payload;
  ProtocolUtil::writef(&stream, kMsgDFileTransfer, static_cast<uint32_t>(mark), &data);

  constexpr size_t kMessageCodeSize = 4;
  return stream.buffer.size() < kMessageCodeSize ? std::string() : stream.buffer.substr(kMessageCodeSize);
}

} // namespace

void FileTransferSenderTests::initTestCase()
{
  m_log.setFilter(LogLevel::Level::Verbose);
}

void FileTransferSenderTests::init()
{
  m_dir = new QTemporaryDir();
  QVERIFY(m_dir->isValid());
}

void FileTransferSenderTests::cleanup()
{
  delete m_dir;
  m_dir = nullptr;
}

QString FileTransferSenderTests::writeFile(const QString &name, const QByteArray &contents)
{
  const QString path = m_dir->filePath(name);
  QFile file(path);
  if (!file.open(QIODevice::WriteOnly)) {
    return {};
  }
  file.write(contents);
  file.close();
  return path;
}

QString FileTransferSenderTests::writeFileOfSize(const QString &name, qint64 size)
{
  QByteArray contents(size, 'p');
  return writeFile(name, contents);
}

void FileTransferSenderTests::emptySelectionEmitsNothing()
{
  RecordingEmitter recorder;
  const auto total = FileTransferSender::send({}, recorder.emitter());

  // An empty drag would tell the peer to expect zero files and then send nothing.
  QCOMPARE(total, static_cast<uint64_t>(0));
  QCOMPARE(recorder.events().size(), static_cast<size_t>(0));
}

void FileTransferSenderTests::announcesBeforeAnyContent()
{
  const auto path = writeFile("a.txt", "content");
  QVERIFY(!path.isEmpty());

  const auto entries = Source::inspectAll({path.toStdString()}, 1024);
  QCOMPARE(entries.size(), static_cast<size_t>(1));

  RecordingEmitter recorder;
  FileTransferSender::send(entries, recorder.emitter());

  // The peer decides where to write from the announcement, so it must arrive first.
  QVERIFY(!recorder.events().empty());
  QCOMPARE(recorder.events().front().kind, SentMessage::Kind::DragInfo);
}

void FileTransferSenderTests::announcementCountMatchesEntries()
{
  const std::vector<std::string> paths = {
      writeFile("one.txt", "1").toStdString(),
      writeFile("two.txt", "2").toStdString(),
      writeFile("three.txt", "3").toStdString(),
  };

  const auto entries = Source::inspectAll(paths, 1024);
  QCOMPARE(entries.size(), static_cast<size_t>(3));

  RecordingEmitter recorder;
  FileTransferSender::send(entries, recorder.emitter());

  QCOMPARE(recorder.events().front().fileCount, static_cast<uint32_t>(3));
}

void FileTransferSenderTests::announcementNamesMatchEntries()
{
  const std::vector<std::string> paths = {
      writeFile("holiday.jpg", "x").toStdString(),
      writeFile("notes.txt", "y").toStdString(),
  };

  const auto entries = Source::inspectAll(paths, 1024);
  RecordingEmitter recorder;
  FileTransferSender::send(entries, recorder.emitter());

  // Names go on the wire as the NUL-separated payload the receiver splits.
  QCOMPARE(
      FileTransferPath::splitNames(recorder.events().front().names),
      (std::vector<std::string>{"holiday.jpg", "notes.txt"})
  );
}

void FileTransferSenderTests::emitsStartThenDataThenEndForOneFile()
{
  const auto path = writeFile("single.txt", "hello transfer");
  QVERIFY(!path.isEmpty());

  const auto entries = Source::inspectAll({path.toStdString()}, 1024);
  RecordingEmitter recorder;
  FileTransferSender::send(entries, recorder.emitter());

  const auto chunks = recorder.chunks();
  QCOMPARE(chunks.size(), static_cast<size_t>(3));
  QCOMPARE(chunks[0].mark, static_cast<uint8_t>(ChunkType::DataStart));
  QCOMPARE(chunks[1].mark, static_cast<uint8_t>(ChunkType::DataChunk));
  QCOMPARE(chunks[2].mark, static_cast<uint8_t>(ChunkType::DataEnd));

  QCOMPARE(chunks[0].payload, std::string("14"));
  QCOMPARE(chunks[1].payload, std::string("hello transfer"));
}

void FileTransferSenderTests::declaredSizeEqualsBytesActuallyStreamed()
{
  // The receiver compares collected bytes against the declared size, so a
  // mismatch would have it reject the file.
  const auto path = writeFileOfSize("sized.bin", static_cast<qint64>(FileChunk::chunkSize()) + 99);
  QVERIFY(!path.isEmpty());

  const auto entries = Source::inspectAll({path.toStdString()}, 8 * 1024 * 1024);
  RecordingEmitter recorder;
  FileTransferSender::send(entries, recorder.emitter());

  const auto chunks = recorder.chunks();
  uint64_t declared = 0;
  uint64_t actual = 0;

  for (const auto &chunk : chunks) {
    switch (chunk.mark) {
    case ChunkType::DataStart:
      declared = std::stoull(chunk.payload);
      break;
    case ChunkType::DataChunk:
      actual += chunk.payload.size();
      break;
    default:
      break;
    }
  }

  QCOMPARE(declared, actual);
}

void FileTransferSenderTests::everyStartedFileIsClosedWithAnEndChunk()
{
  // An unclosed file leaves the peer mid-transfer, and its next start chunk
  // would be refused as "start while a transfer is active".
  const std::vector<std::string> paths = {
      writeFile("a.txt", "aaa").toStdString(),
      writeFile("b.txt", "bbb").toStdString(),
      writeFile("c.txt", "ccc").toStdString(),
  };

  const auto entries = Source::inspectAll(paths, 1024);
  RecordingEmitter recorder;
  FileTransferSender::send(entries, recorder.emitter());

  QCOMPARE(recorder.countOf(static_cast<uint8_t>(ChunkType::DataStart)), static_cast<size_t>(3));
  QCOMPARE(recorder.countOf(static_cast<uint8_t>(ChunkType::DataEnd)), static_cast<size_t>(3));
}

void FileTransferSenderTests::multipleFilesAreSentInOrder()
{
  const std::vector<std::string> paths = {
      writeFile("first.txt", "111").toStdString(),
      writeFile("second.txt", "222").toStdString(),
  };

  const auto entries = Source::inspectAll(paths, 1024);
  RecordingEmitter recorder;
  FileTransferSender::send(entries, recorder.emitter());

  const auto chunks = recorder.chunks();
  QCOMPARE(chunks.size(), static_cast<size_t>(6));

  QCOMPARE(chunks[0].mark, static_cast<uint8_t>(ChunkType::DataStart));
  QCOMPARE(chunks[0].payload, std::string("3"));
  QCOMPARE(chunks[1].payload, std::string("111"));
  QCOMPARE(chunks[2].mark, static_cast<uint8_t>(ChunkType::DataEnd));
  QCOMPARE(chunks[3].mark, static_cast<uint8_t>(ChunkType::DataStart));
  QCOMPARE(chunks[4].payload, std::string("222"));
  QCOMPARE(chunks[5].mark, static_cast<uint8_t>(ChunkType::DataEnd));
}

void FileTransferSenderTests::returnsTotalBytesStreamed()
{
  const std::vector<std::string> paths = {
      writeFile("x.txt", "12345").toStdString(),
      writeFile("y.txt", "abc").toStdString(),
  };

  const auto entries = Source::inspectAll(paths, 1024);
  RecordingEmitter recorder;
  const auto total = FileTransferSender::send(entries, recorder.emitter());

  QCOMPARE(total, static_cast<uint64_t>(8));
}

void FileTransferSenderTests::unopenableFileIsSkippedAndLaterFilesStillTransfer()
{
  const auto first = writeFile("gone-soon.txt", "data");
  const auto second = writeFile("survivor.txt", "still here");
  QVERIFY(!first.isEmpty() && !second.isEmpty());

  auto entries = Source::inspectAll({first.toStdString(), second.toStdString()}, 1024);
  QCOMPARE(entries.size(), static_cast<size_t>(2));

  // Remove the first file after inspection but before sending, so opening fails.
  QVERIFY(QFile::remove(first));

  RecordingEmitter recorder;
  FileTransferSender::send(entries, recorder.emitter());

  // The announcement still lists both, because the protocol has no way to retract
  // one; the surviving file must nevertheless transfer in full.
  QCOMPARE(recorder.events().front().fileCount, static_cast<uint32_t>(2));
  QCOMPARE(recorder.countOf(static_cast<uint8_t>(ChunkType::DataStart)), static_cast<size_t>(1));
  QCOMPARE(recorder.countOf(static_cast<uint8_t>(ChunkType::DataEnd)), static_cast<size_t>(1));
  QCOMPARE(recorder.chunks()[1].payload, std::string("still here"));
}

void FileTransferSenderTests::roundTripThroughFileChunkReassemblesFile()
{
  // The point of the whole layer: what the sender emits must be exactly what the
  // receiver's decoder turns back into the original bytes.
  const std::string contents = "the quick brown fox jumps over the lazy dog";
  const auto path = writeFile("roundtrip.txt", QByteArray::fromStdString(contents));
  QVERIFY(!path.isEmpty());

  const auto entries = Source::inspectAll({path.toStdString()}, 1024);
  RecordingEmitter recorder;
  FileTransferSender::send(entries, recorder.emitter());

  MemoryStream stream;
  for (const auto &chunk : recorder.chunks()) {
    stream.push(encodeChunkForReceiver(chunk.mark, chunk.payload));
  }

  std::string reassembled;
  FileTransferAssemblyState state;
  TransferState result = TransferState::Error;

  for (size_t i = 0; i < recorder.chunks().size(); ++i) {
    result = FileChunk::assemble(&stream, reassembled, state, 1024);
    if (result == TransferState::Finished || result == TransferState::Error) {
      break;
    }
  }

  QCOMPARE(result, TransferState::Finished);
  QCOMPARE(reassembled, contents);
}

void FileTransferSenderTests::roundTripThroughFileChunkReassemblesSeveralFiles()
{
  // Each file must decode independently: a stray byte in one must not bleed into
  // the next, which is what the end/start boundary is for.
  const std::string firstContents(FileChunk::chunkSize() + 5, 'A');
  const std::string secondContents = "short second file";

  const std::vector<std::string> paths = {
      writeFileOfSize("big.bin", static_cast<qint64>(firstContents.size())).toStdString(),
      writeFile("small.txt", QByteArray::fromStdString(secondContents)).toStdString(),
  };

  const auto entries = Source::inspectAll(paths, 8 * 1024 * 1024);
  RecordingEmitter recorder;
  FileTransferSender::send(entries, recorder.emitter());

  MemoryStream stream;
  for (const auto &chunk : recorder.chunks()) {
    stream.push(encodeChunkForReceiver(chunk.mark, chunk.payload));
  }

  std::vector<std::string> decoded;
  std::string current;
  FileTransferAssemblyState state;

  for (size_t i = 0; i < recorder.chunks().size(); ++i) {
    const auto result = FileChunk::assemble(&stream, current, state, 8 * 1024 * 1024);
    if (result == TransferState::Finished) {
      decoded.push_back(current);
      current.clear();
    } else if (result == TransferState::Error) {
      break;
    }
  }

  QCOMPARE(decoded.size(), static_cast<size_t>(2));
  QCOMPARE(decoded[0].size(), firstContents.size());
  QCOMPARE(decoded[1], secondContents);
}

void FileTransferSenderTests::roundTripOfEmptyFileProducesEmptyResult()
{
  // A zero-byte file is legal: start(0), no data chunks, end.
  const auto path = writeFile("empty.dat", "");
  QVERIFY(!path.isEmpty());

  const auto entries = Source::inspectAll({path.toStdString()}, 1024);
  QCOMPARE(entries.size(), static_cast<size_t>(1));

  RecordingEmitter recorder;
  FileTransferSender::send(entries, recorder.emitter());

  const auto chunks = recorder.chunks();
  QCOMPARE(chunks.size(), static_cast<size_t>(2));
  QCOMPARE(chunks[0].payload, std::string("0"));

  MemoryStream stream;
  for (const auto &chunk : chunks) {
    stream.push(encodeChunkForReceiver(chunk.mark, chunk.payload));
  }

  std::string reassembled = "not empty";
  FileTransferAssemblyState state;

  QCOMPARE(FileChunk::assemble(&stream, reassembled, state, 1024), TransferState::Started);
  QCOMPARE(FileChunk::assemble(&stream, reassembled, state, 1024), TransferState::Finished);
  QVERIFY(reassembled.empty());
}

QTEST_MAIN(FileTransferSenderTests)
