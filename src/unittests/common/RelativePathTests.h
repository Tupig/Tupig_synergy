/*
 * Synergy -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 TuPig Synergy Contributors
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include <QTest>

class RelativePathTests : public QObject
{
  Q_OBJECT
private Q_SLOTS:
  void rejectsUnsafePaths();
  void resolveJoinsASafeName();
  void relativeToDirectoryRejectsPathsOutside();
  void canonicalFileMustStayInsideDirectory();
};
