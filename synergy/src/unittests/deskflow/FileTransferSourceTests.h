/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 TuPig
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "base/Log.h"

#include <QTest>
#include <QTemporaryDir>

class FileTransferSourceTests : public QObject
{
  Q_OBJECT
private Q_SLOTS:
  void initTestCase();
  void init();
  void cleanup();

  void inspectAcceptsReadableRegularFile();
  void inspectReportsMissingFile();
  void inspectRejectsDirectory();
  void inspectRejectsFileBeyondLimit();
  void inspectAcceptsFileExactlyAtLimit();
  void inspectReportsBaseNameNotFullPath();
  void inspectAllKeepsGoodEntriesAndReportsRejected();
  void inspectAllAcceptsEmptySelection();

  void dragInfoPayloadIsNulSeparatedNames();
  void dragInfoPayloadIsEmptyForNoEntries();

  void openRefreshesSizeFromHandle();
  void openReportsUnreadableFile();

  void chunkSizeMatchesFileChunk();
  void streamAllReassemblesSingleChunkFile();
  void streamAllReassemblesMultiChunkFile();
  void streamAllProducesNoChunksForEmptyFile();
  void streamAllTotalEqualsDeclaredSize();
  void streamAllEmitsExactChunkBoundaries();
  void remainingCountsDownAsChunksAreRead();
  void closeLeavesSourceNotOpen();
  void readChunkIsEmptyWhenNotOpen();

private:
  QString writeFile(const QString &name, const QByteArray &contents);
  QString writeFileOfSize(const QString &name, qint64 size);

  QTemporaryDir *m_dir = nullptr;
  Log m_log;
};
