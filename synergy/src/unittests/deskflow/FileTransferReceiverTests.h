/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 TuPig
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "base/Log.h"
#include "deskflow/protocol/FileTransferReceiver.h"

#include <QTemporaryDir>
#include <QTest>

#include <memory>
#include <string>
#include <vector>

class FileTransferReceiverTests : public QObject
{
  Q_OBJECT
private Q_SLOTS:
  void initTestCase();
  void init();
  void cleanup();

  // Happy path
  void acceptsAnOrdinaryAnnounce();
  void writesExactContent();
  void writesSeveralFilesInOrder();
  void emptyFileProducesEmptyFileOnDisk();

  // Refusals: names
  void refusesTraversalName();
  void refusesAbsolutePathName();
  void refusesWindowsReservedName();
  void refusesWhenEveryNameIsUnsafe();
  void refusesAnEmptyAnnounce();
  void refusesMoreFilesThanAllowed();

  // Refusals: content
  void refusesFileLargerThanLimit();
  void refusesDeclaredSizeAboveLimit();
  void refusesDataBeyondDeclaredSize();
  void refusesShortTransfer();
  void refusesTransferWithNoAnnouncedName();

  // Disk behaviour
  void neverOverwritesAnExistingFile();
  void duplicateNamesInOneDragDoNotOverwriteEachOther();
  void writesNothingOutsideTheDropDirectory();
  void refusesWhenDropDirectoryIsUnset();

  // State
  void disabledReceiverRefusesEverything();
  void resetClearsPerDragState();
  void refusalsAreCounted();

  // End-to-end, driven by the real sender
  void roundTripFromSenderToDisk();

private:
  //! Feed a hand-built `DDRG` body, then the given `DFTR` bodies.
  void feedAnnounce(const std::vector<std::string> &names);
  void feedChunk(uint8_t mark, const std::string &payload);

  //! Read file bytes from the drop directory.
  QByteArray dropped(const QString &name) const;

  QTemporaryDir *m_dir = nullptr;
  QTemporaryDir *m_drop = nullptr;
  std::unique_ptr<FileTransferReceiver> m_receiver;
  Log m_log;
};
