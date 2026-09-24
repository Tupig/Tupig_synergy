/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 TuPig
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "FileTransferSourceTests.h"

#include "deskflow/protocol/FileChunk.h"
#include "deskflow/protocol/FileTransferPath.h"
#include "deskflow/protocol/FileTransferSource.h"

#include <QDir>
#include <QFile>

#include <string>
#include <vector>

using Source = FileTransferSource;
using Status = FileTransferSource::Status;

namespace {
//! Large enough to be refused by a small limit but cheap to create.
constexpr uint64_t kSmallLimit = 4;
} // namespace

void FileTransferSourceTests::initTestCase()
{
  m_log.setFilter(LogLevel::Level::Verbose);
}

void FileTransferSourceTests::init()
{
  m_dir = new QTemporaryDir();
  QVERIFY(m_dir->isValid());
}

void FileTransferSourceTests::cleanup()
{
  delete m_dir;
  m_dir = nullptr;
}

QString FileTransferSourceTests::writeFile(const QString &name, const QByteArray &contents)
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

QString FileTransferSourceTests::writeFileOfSize(const QString &name, qint64 size)
{
  QByteArray contents(size, '\0');
  for (qint64 i = 0; i < size; ++i) {
    contents[static_cast<int>(i)] = static_cast<char>('a' + (i % 26));
  }
  return writeFile(name, contents);
}

void FileTransferSourceTests::inspectAcceptsReadableRegularFile()
{
  const auto path = writeFile("notes.txt", "hello");
  QVERIFY(!path.isEmpty());

  Source::Entry entry;
  QCOMPARE(Source::inspect(path.toStdString(), 1024, entry), Status::Ok);
  QCOMPARE(entry.size, static_cast<uint64_t>(5));
}

void FileTransferSourceTests::inspectReportsMissingFile()
{
  const auto path = m_dir->filePath("does-not-exist.bin");

  Source::Entry entry;
  QCOMPARE(Source::inspect(path.toStdString(), 1024, entry), Status::NotFound);
}

void FileTransferSourceTests::inspectRejectsDirectory()
{
  // A dragged directory must be refused rather than opened and read as a file.
  const auto path = m_dir->filePath("subdir");
  QVERIFY(QDir().mkpath(path));

  Source::Entry entry;
  QCOMPARE(Source::inspect(path.toStdString(), 1024, entry), Status::NotRegularFile);
}

void FileTransferSourceTests::inspectRejectsFileBeyondLimit()
{
  const auto path = writeFile("big.bin", "way more than four bytes");
  QVERIFY(!path.isEmpty());

  Source::Entry entry;
  QCOMPARE(Source::inspect(path.toStdString(), kSmallLimit, entry), Status::TooLarge);
}

void FileTransferSourceTests::inspectAcceptsFileExactlyAtLimit()
{
  // The limit is inclusive; being off by one here would refuse a legal file.
  const auto path = writeFile("exact.bin", "abcd");
  QVERIFY(!path.isEmpty());

  Source::Entry entry;
  QCOMPARE(Source::inspect(path.toStdString(), kSmallLimit, entry), Status::Ok);
  QCOMPARE(entry.size, kSmallLimit);
}

void FileTransferSourceTests::inspectReportsBaseNameNotFullPath()
{
  // Only the base name goes on the wire: the peer must never learn, or be able
  // to act on, the sender's directory layout.
  const auto path = writeFile("photo.png", "x");
  QVERIFY(!path.isEmpty());

  Source::Entry entry;
  QCOMPARE(Source::inspect(path.toStdString(), 1024, entry), Status::Ok);
  QCOMPARE(entry.name, std::string("photo.png"));
  QVERIFY(entry.name.find('/') == std::string::npos);
  QVERIFY(entry.name.find('\\') == std::string::npos);
}

void FileTransferSourceTests::inspectAllKeepsGoodEntriesAndReportsRejected()
{
  const auto good = writeFile("keep-a.txt", "aaa");
  const auto alsoGood = writeFile("keep-b.txt", "bbb");
  const auto missing = m_dir->filePath("gone.txt");
  const auto tooBig = writeFile("huge.bin", "0123456789");

  std::vector<std::string> rejected;
  std::vector<std::pair<std::string, Status>> reasons;
  const std::vector<std::string> paths = {
      good.toStdString(), missing.toStdString(), alsoGood.toStdString(), tooBig.toStdString()
  };

  const auto entries = Source::inspectAll(paths, kSmallLimit, &reasons);

  // The two readable small files survive, in the order given, and the other two
  // are reported rather than silently dropped.
  QCOMPARE(entries.size(), static_cast<size_t>(2));
  QCOMPARE(entries[0].name, std::string("keep-a.txt"));
  QCOMPARE(entries[1].name, std::string("keep-b.txt"));

  QCOMPARE(reasons.size(), static_cast<size_t>(2));
  QCOMPARE(reasons[0].second, Status::NotFound);
  QCOMPARE(reasons[1].second, Status::TooLarge);
}

void FileTransferSourceTests::inspectAllAcceptsEmptySelection()
{
  QVERIFY(Source::inspectAll({}, kSmallLimit).empty());
}

void FileTransferSourceTests::dragInfoPayloadIsNulSeparatedNames()
{
  std::vector<Source::Entry> entries = {{"/tmp/a", "a.txt", 1}, {"/tmp/b", "b.txt", 2}};

  const auto payload = Source::dragInfoPayload(entries);

  // Must agree with the receiver's splitter, which is the same helper the
  // protocol layer already tests.
  QCOMPARE(FileTransferPath::splitNames(payload), (std::vector<std::string>{"a.txt", "b.txt"}));
}

void FileTransferSourceTests::dragInfoPayloadIsEmptyForNoEntries()
{
  QVERIFY(Source::dragInfoPayload({}).empty());
}

void FileTransferSourceTests::openRefreshesSizeFromHandle()
{
  const auto path = writeFile("open-me.txt", "0123456789");
  QVERIFY(!path.isEmpty());

  Source::Entry entry;
  QCOMPARE(Source::inspect(path.toStdString(), 1024, entry), Status::Ok);

  // A stale size would make the peer reject the transfer, so open() must take
  // the authoritative value rather than trusting the earlier stat.
  entry.size = 0;
  Source source;
  QCOMPARE(source.open(entry), Status::Ok);
  QCOMPARE(entry.size, static_cast<uint64_t>(10));
  QVERIFY(source.isOpen());

  source.close();
}

void FileTransferSourceTests::openReportsUnreadableFile()
{
  const auto path = m_dir->filePath("vanished.txt");

  Source::Entry entry;
  entry.path = path.toStdString();
  entry.name = "vanished.txt";

  Source source;
  QCOMPARE(source.open(entry), Status::NotReadable);
  QVERIFY(!source.isOpen());
}

void FileTransferSourceTests::chunkSizeMatchesFileChunk()
{
  const auto path = writeFileOfSize("chunked.bin", static_cast<qint64>(FileChunk::chunkSize()) * 2 + 7);
  QVERIFY(!path.isEmpty());

  Source::Entry entry;
  QCOMPARE(Source::inspect(path.toStdString(), 8 * 1024 * 1024, entry), Status::Ok);

  Source source;
  QCOMPARE(source.open(entry), Status::Ok);

  // Every chunk must fit the transport ceiling; one larger would be rejected by
  // the peer as a protocol error and drop the connection.
  while (source.remaining() > 0) {
    const auto chunk = source.readChunk();
    QVERIFY(!chunk.empty());
    QVERIFY(chunk.size() <= FileChunk::chunkSize());
  }
}

void FileTransferSourceTests::streamAllReassemblesSingleChunkFile()
{
  const std::string contents = "the quick brown fox";
  const auto path = writeFile("single.txt", QByteArray::fromStdString(contents));
  QVERIFY(!path.isEmpty());

  Source::Entry entry;
  QCOMPARE(Source::inspect(path.toStdString(), 1024, entry), Status::Ok);

  Source source;
  QCOMPARE(source.open(entry), Status::Ok);

  std::string reassembled;
  const auto total = source.streamAll([&](const std::string &chunk) { reassembled += chunk; });

  QCOMPARE(total, static_cast<uint64_t>(contents.size()));
  QCOMPARE(reassembled, contents);
}

void FileTransferSourceTests::streamAllReassemblesMultiChunkFile()
{
  const auto size = static_cast<qint64>(FileChunk::chunkSize()) * 2 + 7;
  const auto path = writeFileOfSize("multi.bin", size);
  QVERIFY(!path.isEmpty());

  Source::Entry entry;
  QCOMPARE(Source::inspect(path.toStdString(), 8 * 1024 * 1024, entry), Status::Ok);
  QCOMPARE(entry.size, static_cast<uint64_t>(size));

  Source source;
  QCOMPARE(source.open(entry), Status::Ok);

  std::string reassembled;
  size_t chunkCount = 0;
  const auto total = source.streamAll([&](const std::string &chunk) {
    ++chunkCount;
    reassembled += chunk;
  });

  QCOMPARE(total, static_cast<uint64_t>(size));
  QCOMPARE(reassembled.size(), static_cast<size_t>(size));
  QCOMPARE(chunkCount, static_cast<size_t>(3));
}

void FileTransferSourceTests::streamAllProducesNoChunksForEmptyFile()
{
  // A zero-byte file is legal: no data chunks at all, just start and end.
  const auto path = writeFile("empty.txt", "");
  QVERIFY(!path.isEmpty());

  Source::Entry entry;
  QCOMPARE(Source::inspect(path.toStdString(), 1024, entry), Status::Ok);
  QCOMPARE(entry.size, static_cast<uint64_t>(0));

  Source source;
  QCOMPARE(source.open(entry), Status::Ok);

  size_t chunkCount = 0;
  const auto total = source.streamAll([&](const std::string &) { ++chunkCount; });

  QCOMPARE(total, static_cast<uint64_t>(0));
  QCOMPARE(chunkCount, static_cast<size_t>(0));
}

void FileTransferSourceTests::streamAllTotalEqualsDeclaredSize()
{
  // The receiver compares the bytes it collected against the size in the start
  // chunk, so the number of bytes streamed must equal the size open() reported.
  const auto size = static_cast<qint64>(FileChunk::chunkSize()) + 123;
  const auto path = writeFileOfSize("contract.bin", size);
  QVERIFY(!path.isEmpty());

  Source::Entry entry;
  QCOMPARE(Source::inspect(path.toStdString(), 8 * 1024 * 1024, entry), Status::Ok);

  Source source;
  QCOMPARE(source.open(entry), Status::Ok);
  const auto declared = entry.size;

  uint64_t streamed = 0;
  source.streamAll([&](const std::string &chunk) { streamed += chunk.size(); });

  QCOMPARE(streamed, declared);
}

void FileTransferSourceTests::streamAllEmitsExactChunkBoundaries()
{
  const auto chunkSize = FileChunk::chunkSize();
  const auto size = chunkSize * 2;
  const auto path = writeFileOfSize("bounds.bin", static_cast<qint64>(size));
  QVERIFY(!path.isEmpty());

  Source::Entry entry;
  QCOMPARE(Source::inspect(path.toStdString(), 8 * 1024 * 1024, entry), Status::Ok);

  Source source;
  QCOMPARE(source.open(entry), Status::Ok);

  std::vector<size_t> sizes;
  source.streamAll([&](const std::string &chunk) { sizes.push_back(chunk.size()); });

  // Exactly full chunks, and no trailing empty chunk once the file is consumed.
  QCOMPARE(sizes.size(), static_cast<size_t>(2));
  QCOMPARE(sizes[0], chunkSize);
  QCOMPARE(sizes[1], chunkSize);
}

void FileTransferSourceTests::remainingCountsDownAsChunksAreRead()
{
  const auto size = static_cast<qint64>(FileChunk::chunkSize()) + 10;
  const auto path = writeFileOfSize("countdown.bin", size);
  QVERIFY(!path.isEmpty());

  Source::Entry entry;
  QCOMPARE(Source::inspect(path.toStdString(), 8 * 1024 * 1024, entry), Status::Ok);

  Source source;
  QCOMPARE(source.open(entry), Status::Ok);
  QCOMPARE(source.remaining(), static_cast<uint64_t>(size));

  source.readChunk();
  QCOMPARE(source.remaining(), static_cast<uint64_t>(10));

  source.readChunk();
  QCOMPARE(source.remaining(), static_cast<uint64_t>(0));
}

void FileTransferSourceTests::closeLeavesSourceNotOpen()
{
  const auto path = writeFile("closable.txt", "data");
  QVERIFY(!path.isEmpty());

  Source::Entry entry;
  QCOMPARE(Source::inspect(path.toStdString(), 1024, entry), Status::Ok);

  Source source;
  QCOMPARE(source.open(entry), Status::Ok);
  QVERIFY(source.isOpen());

  source.close();
  QVERIFY(!source.isOpen());
  QCOMPARE(source.remaining(), static_cast<uint64_t>(0));

  // Closing twice must be harmless.
  source.close();
  QVERIFY(!source.isOpen());
}

void FileTransferSourceTests::readChunkIsEmptyWhenNotOpen()
{
  Source source;
  QVERIFY(!source.isOpen());
  QVERIFY(source.readChunk().empty());
  QCOMPARE(source.remaining(), static_cast<uint64_t>(0));
}

QTEST_MAIN(FileTransferSourceTests)
