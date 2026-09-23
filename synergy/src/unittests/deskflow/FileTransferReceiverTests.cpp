/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 TuPig
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "FileTransferReceiverTests.h"

#include "ProtocolTypes.h"
#include "ProtocolUtil.h"
#include "deskflow/protocol/FileTransferPath.h"
#include "deskflow/protocol/FileTransferReceiver.h"
#include "deskflow/protocol/FileTransferSender.h"
#include "deskflow/protocol/FileTransferSource.h"
#include "io/IStream.h"

#include <QDir>
#include <QFile>

#include <algorithm>
#include <cstring>
#include <deque>
#include <memory>
#include <string>
#include <vector>

namespace {

//! A stream the receiver reads message bodies from.
/*!
The receiver expects to be positioned just past the 4-byte message code, so the
code is stripped when messages are pushed - the same contract the real dispatchers
honour.
*/
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

//! Captures what the sender writes, so the exact wire bytes can be replayed.
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

//! Encode a `DDRG` body (no message code), as the dispatcher would leave it.
std::string encodeDragInfo(uint32_t fileCount, const std::string &payload)
{
  BufferWriteStream stream;
  auto data = payload;
  ProtocolUtil::writef(&stream, kMsgDDragInfo, fileCount, &data);

  constexpr size_t kMessageCodeSize = 4;
  return stream.str().substr(kMessageCodeSize);
}

//! Encode a `DFTR` body (no message code).
std::string encodeFileChunk(uint8_t mark, const std::string &payload)
{
  BufferWriteStream stream;
  auto data = payload;
  ProtocolUtil::writef(&stream, kMsgDFileTransfer, static_cast<uint32_t>(mark), &data);

  constexpr size_t kMessageCodeSize = 4;
  return stream.str().substr(kMessageCodeSize);
}

//! A stream that swallows writes, for driving the sender without a network.
class NullWriteStream : public deskflow::IStream
{
public:
  void close() override
  {
  }
  uint32_t read(void *, uint32_t) override
  {
    return 0;
  }
  void write(const void *, uint32_t) override
  {
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
    return const_cast<NullWriteStream *>(this);
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

} // namespace

void FileTransferReceiverTests::initTestCase()
{
  m_log.setFilter(LogLevel::Level::Verbose);
}

void FileTransferReceiverTests::init()
{
  m_dir = new QTemporaryDir();
  QVERIFY(m_dir->isValid());

  m_drop = new QTemporaryDir();
  QVERIFY(m_drop->isValid());

  FileTransferReceiver::Options options;
  options.dropDirectory = m_drop->path().toStdString();
  m_receiver = std::make_unique<FileTransferReceiver>(options);
}

void FileTransferReceiverTests::cleanup()
{
  m_receiver.reset();
  delete m_dir;
  m_dir = nullptr;
  delete m_drop;
  m_drop = nullptr;
}

QByteArray FileTransferReceiverTests::dropped(const QString &name) const
{
  QFile file(QDir(m_drop->path()).filePath(name));
  if (!file.open(QIODevice::ReadOnly)) {
    return {};
  }
  return file.readAll();
}

void FileTransferReceiverTests::feedAnnounce(const std::vector<std::string> &names)
{
  MemoryStream stream;
  stream.push(encodeDragInfo(static_cast<uint32_t>(names.size()), FileTransferPath::joinNames(names)));
  m_receiver->onDragInfo(&stream);
}

void FileTransferReceiverTests::feedChunk(uint8_t mark, const std::string &payload)
{
  MemoryStream stream;
  stream.push(encodeFileChunk(mark, payload));
  m_receiver->onFileChunk(&stream);
}

void FileTransferReceiverTests::acceptsAnOrdinaryAnnounce()
{
  feedAnnounce({"holiday.jpg"});

  QCOMPARE(m_receiver->acceptedNames().size(), static_cast<size_t>(1));
  QCOMPARE(m_receiver->acceptedNames()[0], std::string("holiday.jpg"));
  QCOMPARE(m_receiver->refusedCount(), static_cast<size_t>(0));
}

void FileTransferReceiverTests::writesExactContent()
{
  const std::string content = "the quick brown fox";
  feedAnnounce({"note.txt"});
  feedChunk(ChunkType::DataStart, std::to_string(content.size()));
  feedChunk(ChunkType::DataChunk, content);
  feedChunk(ChunkType::DataEnd, "");

  QCOMPARE(m_receiver->writtenFiles().size(), static_cast<size_t>(1));
  QCOMPARE(dropped("note.txt"), QByteArray::fromStdString(content));
}

void FileTransferReceiverTests::writesSeveralFilesInOrder()
{
  // Names are not carried per file, so the Nth transfer must land on the Nth name.
  feedAnnounce({"one.txt", "two.txt"});

  feedChunk(ChunkType::DataStart, "3");
  feedChunk(ChunkType::DataChunk, "111");
  feedChunk(ChunkType::DataEnd, "");

  feedChunk(ChunkType::DataStart, "3");
  feedChunk(ChunkType::DataChunk, "222");
  feedChunk(ChunkType::DataEnd, "");

  QCOMPARE(dropped("one.txt"), QByteArray("111"));
  QCOMPARE(dropped("two.txt"), QByteArray("222"));
  QCOMPARE(m_receiver->writtenFiles().size(), static_cast<size_t>(2));
}

void FileTransferReceiverTests::emptyFileProducesEmptyFileOnDisk()
{
  feedAnnounce({"empty.dat"});
  feedChunk(ChunkType::DataStart, "0");
  feedChunk(ChunkType::DataEnd, "");

  QCOMPARE(m_receiver->writtenFiles().size(), static_cast<size_t>(1));
  QVERIFY(QFile::exists(QDir(m_drop->path()).filePath("empty.dat")));
  QVERIFY(dropped("empty.dat").isEmpty());
}

void FileTransferReceiverTests::refusesTraversalName()
{
  feedAnnounce({"../../escaped.txt"});

  QVERIFY(m_receiver->acceptedNames().empty());
  QCOMPARE(m_receiver->refusedCount(), static_cast<size_t>(1));

  // Nothing may be created in the parent of the drop directory.
  const QDir parent(m_drop->path() + "/..");
  QVERIFY(!QFile::exists(parent.filePath("escaped.txt")));
}

void FileTransferReceiverTests::refusesAbsolutePathName()
{
  feedAnnounce({"/tmp/absolute-escape.txt"});
  QVERIFY(m_receiver->acceptedNames().empty());
  QCOMPARE(m_receiver->refusedCount(), static_cast<size_t>(1));
}

void FileTransferReceiverTests::refusesWindowsReservedName()
{
  feedAnnounce({"CON"});
  QVERIFY(m_receiver->acceptedNames().empty());
  QCOMPARE(m_receiver->refusedCount(), static_cast<size_t>(1));
}

void FileTransferReceiverTests::refusesWhenEveryNameIsUnsafe()
{
  feedAnnounce({"../a", "b/c", ".."});
  QVERIFY(m_receiver->acceptedNames().empty());
  QCOMPARE(m_receiver->refusedCount(), static_cast<size_t>(3));
}

void FileTransferReceiverTests::refusesAnEmptyAnnounce()
{
  MemoryStream stream;
  stream.push(encodeDragInfo(0, std::string()));
  QVERIFY(!m_receiver->onDragInfo(&stream));
  QVERIFY(m_receiver->acceptedNames().empty());
}

void FileTransferReceiverTests::refusesMoreFilesThanAllowed()
{
  FileTransferReceiver::Options options;
  options.dropDirectory = m_drop->path().toStdString();
  options.maxFileCount = 2;
  FileTransferReceiver receiver(options);

  MemoryStream stream;
  stream.push(encodeDragInfo(3, FileTransferPath::joinNames({"a", "b", "c"})));

  QVERIFY(!receiver.onDragInfo(&stream));
  QVERIFY(receiver.acceptedNames().empty());
  QCOMPARE(receiver.refusedCount(), static_cast<size_t>(3));
}

void FileTransferReceiverTests::refusesFileLargerThanLimit()
{
  FileTransferReceiver::Options options;
  options.dropDirectory = m_drop->path().toStdString();
  options.maxFileSize = 4;
  FileTransferReceiver receiver(options);

  MemoryStream announce;
  announce.push(encodeDragInfo(1, FileTransferPath::joinNames({"big.bin"})));
  QVERIFY(receiver.onDragInfo(&announce));

  MemoryStream startChunk;
  startChunk.push(encodeFileChunk(ChunkType::DataStart, "5"));
  QCOMPARE(receiver.onFileChunk(&startChunk), TransferState::Error);

  QVERIFY(receiver.writtenFiles().empty());
}

void FileTransferReceiverTests::refusesDeclaredSizeAboveLimit()
{
  feedAnnounce({"huge.bin"});

  // Declared size is a promise, but it is also the receiver's only chance to
  // refuse an oversized file before any of it arrives.
  feedChunk(ChunkType::DataStart, std::to_string(64ull * 1024 * 1024 * 4));

  QVERIFY(m_receiver->writtenFiles().empty());
  QVERIFY(m_receiver->refusedCount() >= 1);
}

void FileTransferReceiverTests::refusesDataBeyondDeclaredSize()
{
  feedAnnounce({"liar.txt"});
  feedChunk(ChunkType::DataStart, "2");
  feedChunk(ChunkType::DataChunk, "AAAA");

  QVERIFY(m_receiver->writtenFiles().empty());

  // And no staged file may be left behind.
  const QDir dropDir(m_drop->path());
  const auto leftovers = dropDir.entryList({"*.part"}, QDir::Files);
  QVERIFY(leftovers.isEmpty());
}

void FileTransferReceiverTests::refusesShortTransfer()
{
  feedAnnounce({"short.txt"});
  feedChunk(ChunkType::DataStart, "10");
  feedChunk(ChunkType::DataChunk, "abc");
  feedChunk(ChunkType::DataEnd, "");

  QVERIFY(m_receiver->writtenFiles().empty());
}

void FileTransferReceiverTests::refusesTransferWithNoAnnouncedName()
{
  // More transfers than names: the peer is not following the protocol.
  feedAnnounce({"only.txt"});

  feedChunk(ChunkType::DataStart, "1");
  feedChunk(ChunkType::DataChunk, "x");
  feedChunk(ChunkType::DataEnd, "");

  feedChunk(ChunkType::DataStart, "1");
  feedChunk(ChunkType::DataChunk, "y");
  feedChunk(ChunkType::DataEnd, "");

  QCOMPARE(m_receiver->writtenFiles().size(), static_cast<size_t>(1));
  QVERIFY(m_receiver->refusedCount() >= 1);
}

void FileTransferReceiverTests::neverOverwritesAnExistingFile()
{
  const QString existing = QDir(m_drop->path()).filePath("note.txt");
  {
    QFile file(existing);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write("LOCAL ORIGINAL");
  }

  feedAnnounce({"note.txt"});
  feedChunk(ChunkType::DataStart, "8");
  feedChunk(ChunkType::DataChunk, "incoming");
  feedChunk(ChunkType::DataEnd, "");

  // The local file must survive untouched; the incoming one takes a new name.
  QCOMPARE(dropped("note.txt"), QByteArray("LOCAL ORIGINAL"));
  QCOMPARE(dropped("note (1).txt"), QByteArray("incoming"));
}

void FileTransferReceiverTests::writesNothingOutsideTheDropDirectory()
{
  // A grab-bag of hostile names, then a legitimate transfer: whatever is refused
  // must not appear anywhere, and the legitimate file must still land.
  feedAnnounce({"../../up.txt", "C:\\Windows\\down.txt", "ok.txt"});
  QCOMPARE(m_receiver->acceptedNames().size(), static_cast<size_t>(1));

  feedChunk(ChunkType::DataStart, "2");
  feedChunk(ChunkType::DataChunk, "ok");
  feedChunk(ChunkType::DataEnd, "");

  QCOMPARE(dropped("ok.txt"), QByteArray("ok"));
  QVERIFY(!QFile::exists(QDir(m_drop->path() + "/..").filePath("up.txt")));
  QVERIFY(!QFile::exists(QDir(m_drop->path()).filePath("down.txt")));
}

void FileTransferReceiverTests::refusesWhenDropDirectoryIsUnset()
{
  // Regression guard: an empty drop directory previously produced a RELATIVE
  // target path ("a.txt"), because path("") / name does not fail - so the file
  // was written into the process working directory instead of being refused.
  FileTransferReceiver::Options options;
  options.dropDirectory = "";
  FileTransferReceiver receiver(options);

  const QString stray = QDir::current().filePath("a.txt");
  const bool strayExistedBefore = QFile::exists(stray);

  MemoryStream announce;
  announce.push(encodeDragInfo(1, FileTransferPath::joinNames({"a.txt"})));

  // Nothing can be accepted without somewhere safe to put it.
  QVERIFY(!receiver.onDragInfo(&announce));
  QVERIFY(receiver.acceptedNames().empty());

  MemoryStream startChunk;
  startChunk.push(encodeFileChunk(ChunkType::DataStart, "1"));
  receiver.onFileChunk(&startChunk);

  MemoryStream dataChunk;
  dataChunk.push(encodeFileChunk(ChunkType::DataChunk, "x"));
  receiver.onFileChunk(&dataChunk);

  MemoryStream endChunk;
  endChunk.push(encodeFileChunk(ChunkType::DataEnd, ""));
  receiver.onFileChunk(&endChunk);

  QVERIFY(receiver.writtenFiles().empty());

  // And critically: no stray file in the working directory.
  QCOMPARE(QFile::exists(stray), strayExistedBefore);
}

void FileTransferReceiverTests::disabledReceiverRefusesEverything()
{
  FileTransferReceiver::Options options;
  options.dropDirectory = m_drop->path().toStdString();
  options.enabled = false;
  FileTransferReceiver receiver(options);

  MemoryStream announce;
  announce.push(encodeDragInfo(1, FileTransferPath::joinNames({"a.txt"})));
  QVERIFY(!receiver.onDragInfo(&announce));
  QVERIFY(receiver.acceptedNames().empty());

  MemoryStream startChunk;
  startChunk.push(encodeFileChunk(ChunkType::DataStart, "1"));
  QCOMPARE(receiver.onFileChunk(&startChunk), TransferState::Error);
}

void FileTransferReceiverTests::resetClearsPerDragState()
{
  feedAnnounce({"one.txt"});
  feedChunk(ChunkType::DataStart, "3");
  feedChunk(ChunkType::DataChunk, "abc");
  feedChunk(ChunkType::DataEnd, "");
  QCOMPARE(m_receiver->acceptedNames().size(), static_cast<size_t>(1));

  m_receiver->reset();
  QVERIFY(m_receiver->acceptedNames().empty());

  // A later drag must start from a clean slate rather than continuing the old
  // file index.
  feedAnnounce({"two.txt"});
  feedChunk(ChunkType::DataStart, "2");
  feedChunk(ChunkType::DataChunk, "hi");
  feedChunk(ChunkType::DataEnd, "");

  QCOMPARE(dropped("two.txt"), QByteArray("hi"));
}

void FileTransferReceiverTests::refusalsAreCounted()
{
  feedAnnounce({"../bad", "good.txt"});
  QCOMPARE(m_receiver->refusedCount(), static_cast<size_t>(1));

  // A malformed transfer adds to the count.
  feedChunk(ChunkType::DataStart, "10");
  feedChunk(ChunkType::DataEnd, "");
  QVERIFY(m_receiver->refusedCount() >= 2);
}

void FileTransferReceiverTests::roundTripFromSenderToDisk()
{
  // The strongest check available here: real sender, real files, real disk.
  const std::string contents(FileChunk::chunkSize() + 777, 'R');

  const QString source = QDir(m_dir->path()).filePath("payload.bin");
  {
    QFile file(source);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write(QByteArray::fromStdString(contents));
  }

  const auto entries = FileTransferSource::inspectAll({source.toStdString()}, 16 * 1024 * 1024);
  QCOMPARE(entries.size(), static_cast<size_t>(1));

  // Collect the sender's messages, dropping each message code the way the
  // dispatcher does before the receiver sees it.
  std::vector<std::string> messages;
  FileTransferSender::Emitter emitter{
      [&messages](uint32_t count, const std::string &names) { messages.push_back(encodeDragInfo(count, names)); },
      [&messages](uint8_t mark, const std::string &payload) { messages.push_back(encodeFileChunk(mark, payload)); }
  };

  const auto streamed = FileTransferSender::send(entries, emitter);
  QCOMPARE(streamed, static_cast<uint64_t>(contents.size()));
  QVERIFY(!messages.empty());

  MemoryStream stream;
  for (const auto &message : messages) {
    stream.push(message);
  }

  QVERIFY(m_receiver->onDragInfo(&stream));
  QCOMPARE(m_receiver->acceptedNames().size(), static_cast<size_t>(1));

  TransferState result = TransferState::Error;
  for (size_t i = 0; i + 1 < messages.size(); ++i) {
    result = m_receiver->onFileChunk(&stream);
    if (result == TransferState::Finished || result == TransferState::Error) {
      break;
    }
  }

  QCOMPARE(result, TransferState::Finished);
  QCOMPARE(m_receiver->writtenFiles().size(), static_cast<size_t>(1));
  QCOMPARE(dropped("payload.bin"), QByteArray::fromStdString(contents));
}

QTEST_MAIN(FileTransferReceiverTests)
