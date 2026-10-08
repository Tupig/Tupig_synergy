/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 TuPig
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "base/Log.h"

#include <QTemporaryDir>
#include <QTest>

class FileTransferSenderTests : public QObject
{
  Q_OBJECT
private Q_SLOTS:
  void initTestCase();
  void init();
  void cleanup();

  void emptySelectionEmitsNothing();
  void announcesBeforeAnyContent();
  void announcementCountMatchesEntries();
  void announcementNamesMatchEntries();
  void emitsStartThenDataThenEndForOneFile();
  void declaredSizeEqualsBytesActuallyStreamed();
  void everyStartedFileIsClosedWithAnEndChunk();
  void multipleFilesAreSentInOrder();
  void returnsTotalBytesStreamed();
  void unopenableFileIsSkippedAndLaterFilesStillTransfer();

  void roundTripThroughFileChunkReassemblesFile();
  void roundTripThroughFileChunkReassemblesSeveralFiles();
  void roundTripOfEmptyFileProducesEmptyResult();

private:
  QString writeFile(const QString &name, const QByteArray &contents);
  QString writeFileOfSize(const QString &name, qint64 size);

  QTemporaryDir *m_dir = nullptr;
  Log m_log;
};
