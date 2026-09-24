/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 TuPig
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "FileTransferOutboundTests.h"

#include "deskflow/protocol/FileTransferOutbound.h"

#include <QFile>
#include <string>
#include <vector>

namespace {
struct SentMessage
{
  enum class Kind
  {
    DragInfo,
    Chunk
  };
  Kind kind = Kind::Chunk;
  uint32_t fileCount = 0;
  std::string payload;
  uint8_t mark = 0;
};
} // namespace

void FileTransferOutboundTests::initTestCase()
{
  m_log.setFilter(LogLevel::Level::Verbose);
}

void FileTransferOutboundTests::init()
{
  QVERIFY(m_dir.isValid() || m_dir.remove() || m_dir.isValid());
}

void FileTransferOutboundTests::cleanup()
{
  // QTemporaryDir cleans itself.
}

QString FileTransferOutboundTests::writeFile(const QString &name, const QByteArray &contents)
{
  const QString path = m_dir.filePath(name);
  QFile file(path);
  if (!file.open(QIODevice::WriteOnly)) {
    return {};
  }
  if (file.write(contents) != contents.size()) {
    return {};
  }
  file.close();
  return path;
}

void FileTransferOutboundTests::disabledSendsNothing()
{
  const auto path = writeFile("a.txt", "hello");
  QVERIFY(!path.isEmpty());
  std::vector<SentMessage> sent;

  FileTransferSender::Emitter emitter;
  emitter.dragInfo = [&](uint32_t count, const std::string &names) {
    sent.push_back({SentMessage::Kind::DragInfo, count, names, 0});
  };
  emitter.chunk = [&](uint8_t mark, const std::string &payload) {
    sent.push_back({SentMessage::Kind::Chunk, 0, payload, mark});
  };

  FileTransferOutbound::Options options;
  options.enabled = false;

  QCOMPARE(FileTransferOutbound::sendPaths({path.toStdString()}, options, emitter), uint64_t{0});
  QVERIFY(sent.empty());
}

void FileTransferOutboundTests::emptyPathsSendNothing()
{
  std::vector<SentMessage> sent;
  FileTransferSender::Emitter emitter;
  emitter.dragInfo = [&](uint32_t, const std::string &) { sent.push_back({}); };
  emitter.chunk = [&](uint8_t, const std::string &) { sent.push_back({}); };

  FileTransferOutbound::Options options;
  QCOMPARE(FileTransferOutbound::sendPaths({}, options, emitter), uint64_t{0});
  QVERIFY(sent.empty());
}

void FileTransferOutboundTests::sendsReadableFiles()
{
  const auto path = writeFile("note.txt", "abc");
  QVERIFY(!path.isEmpty());
  std::vector<SentMessage> sent;

  FileTransferSender::Emitter emitter;
  emitter.dragInfo = [&](uint32_t count, const std::string &names) {
    sent.push_back({SentMessage::Kind::DragInfo, count, names, 0});
  };
  emitter.chunk = [&](uint8_t mark, const std::string &payload) {
    sent.push_back({SentMessage::Kind::Chunk, 0, payload, mark});
  };

  FileTransferOutbound::Options options;
  const auto bytes = FileTransferOutbound::sendPaths({path.toStdString()}, options, emitter);
  QCOMPARE(bytes, uint64_t{3});
  QVERIFY(!sent.empty());
  QCOMPARE(sent.front().kind, SentMessage::Kind::DragInfo);
  QCOMPARE(sent.front().fileCount, uint32_t{1});
}

void FileTransferOutboundTests::trimsToMaxFileCount()
{
  std::vector<std::string> paths;
  for (int i = 0; i < 5; ++i) {
    const auto path = writeFile(QString("f%1.txt").arg(i), "x");
    QVERIFY(!path.isEmpty());
    paths.push_back(path.toStdString());
  }

  uint32_t announced = 0;
  FileTransferSender::Emitter emitter;
  emitter.dragInfo = [&](uint32_t count, const std::string &) { announced = count; };
  emitter.chunk = [&](uint8_t, const std::string &) {};

  FileTransferOutbound::Options options;
  options.maxFileCount = 2;

  QVERIFY(FileTransferOutbound::sendPaths(paths, options, emitter) > 0);
  QCOMPARE(announced, uint32_t{2});
}

void FileTransferOutboundTests::dropsFilesAboveMaxSize()
{
  const auto big = writeFile("big.bin", QByteArray(32, 'Z'));
  const auto small = writeFile("small.txt", "ok");
  QVERIFY(!big.isEmpty());
  QVERIFY(!small.isEmpty());

  uint32_t announced = 0;
  std::string names;
  FileTransferSender::Emitter emitter;
  emitter.dragInfo = [&](uint32_t count, const std::string &payload) {
    announced = count;
    names = payload;
  };
  emitter.chunk = [&](uint8_t, const std::string &) {};

  FileTransferOutbound::Options options;
  options.maxFileSize = 8;

  QCOMPARE(FileTransferOutbound::sendPaths({big.toStdString(), small.toStdString()}, options, emitter), uint64_t{2});
  QCOMPARE(announced, uint32_t{1});
  QVERIFY(names.find("small.txt") != std::string::npos);
  QVERIFY(names.find("big.bin") == std::string::npos);
}

QTEST_MAIN(FileTransferOutboundTests)
#include "FileTransferOutboundTests.moc"
