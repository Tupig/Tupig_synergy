/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 TuPig
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "base/Log.h"

#include <QTest>

class FileTransferPathTests : public QObject
{
  Q_OBJECT
private Q_SLOTS:
  void initTestCase();
  void acceptsOrdinaryName();
  void acceptsUnicodeName();
  void rejectsEmptyName();
  void rejectsParentDirectoryReferences();
  void rejectsUnixPathTraversal();
  void rejectsWindowsPathTraversal();
  void rejectsAbsolutePosixPath();
  void rejectsDriveQualifiedPath();
  void rejectsControlCharacters();
  void rejectsNulEmbeddedName();
  void rejectsWindowsReservedDeviceNames();
  void rejectsReservedDeviceNameWithExtension();
  void rejectsTrailingDotOrSpace();
  void rejectsWindowsForbiddenCharacters();
  void rejectsOverlongName();
  void acceptsNameThatMerelyContainsDots();
  void joinUnderDirectoryAddsSeparatorOnce();
  void joinNamesRoundTripsThroughSplit();
  void splitNamesDropsEmptySegments();

private:
  Log m_log;
};
