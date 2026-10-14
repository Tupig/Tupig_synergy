/*
 * Synergy -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 TuPig Synergy Contributors
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <QTest>

class IpcTokenTests : public QObject
{
  Q_OBJECT
private Q_SLOTS:
  void initTestCase();
  void cleanupTestCase();

  void generateProduces64LowercaseHex();
  void writeRejectsEmptyToken();
  void readMissingFileReturnsEmpty();
  void writeThenReadRoundTrips();
  void writeReplacesExistingToken();
  void tokenPathIsUnderSettingsPath();

#ifndef Q_OS_WIN
  void writeSetsOwnerOnlyPermissions();
  void refreshRestoresOwnerOnlyPermissions();
#else
  void writeAppliesRestrictedDacl();
#endif

private:
  QString m_testDir;
  const QString m_serverName = QStringLiteral("ipc-token-tests");
};
