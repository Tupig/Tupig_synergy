/*
 * Synergy -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 TuPig Synergy Contributors
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "RelativePathTests.h"

#include "common/RelativePath.h"

#include <QDir>
#include <QFile>

void RelativePathTests::rejectsUnsafePaths()
{
  QVERIFY(synergy::isSafeRelativePath(QStringLiteral("tool.exe")));
  QVERIFY(synergy::isSafeRelativePath(QStringLiteral("dir/tool.exe")));
  QVERIFY(!synergy::isSafeRelativePath(QString()));
  QVERIFY(!synergy::isSafeRelativePath(QStringLiteral(".")));
  QVERIFY(!synergy::isSafeRelativePath(QStringLiteral("..")));
  QVERIFY(!synergy::isSafeRelativePath(QStringLiteral("../tool.exe")));
  QVERIFY(!synergy::isSafeRelativePath(QStringLiteral("dir/../tool.exe")));
  QVERIFY(!synergy::isSafeRelativePath(QStringLiteral("/etc/passwd")));
  QVERIFY(!synergy::isSafeRelativePath(QStringLiteral("\\Windows\\notepad.exe")));
  QVERIFY(!synergy::isSafeRelativePath(QStringLiteral("C:/Windows/notepad.exe")));
  QVERIFY(!synergy::isSafeRelativePath(QStringLiteral("\\\\server\\share")));
  QVERIFY(!synergy::isSafeRelativePath(QStringLiteral("tool.exe\n")));
}

void RelativePathTests::resolveJoinsASafeName()
{
  const auto joined = synergy::resolveRelativePath(QStringLiteral("settings"), QStringLiteral("tool.exe"));
  QCOMPARE(QDir::fromNativeSeparators(joined), QStringLiteral("settings/tool.exe"));
  QVERIFY(synergy::resolveRelativePath(QStringLiteral("settings"), QStringLiteral("..\\tool.exe")).isEmpty());
}

void RelativePathTests::relativeToDirectoryRejectsPathsOutside()
{
  const auto root = QDir::tempPath();
  const auto inside = QDir(root).filePath(QStringLiteral("synergy-relative-path-test.txt"));
  QCOMPARE(
      QDir::fromNativeSeparators(synergy::relativeToDirectory(root, inside)),
      QStringLiteral("synergy-relative-path-test.txt")
  );
  QVERIFY(synergy::relativeToDirectory(root, QDir::rootPath()).isEmpty());
}

void RelativePathTests::canonicalFileMustStayInsideDirectory()
{
  const auto root = QDir(QDir::tempPath()).filePath(QStringLiteral("synergy-canonical-root"));
  const auto sibling = QDir(QDir::tempPath()).filePath(QStringLiteral("synergy-canonical-root-outside"));
  QDir().mkpath(root);
  QDir().mkpath(sibling);

  const auto inside = QDir(root).filePath(QStringLiteral("tool.exe"));
  const auto outside = QDir(sibling).filePath(QStringLiteral("tool.exe"));
  QFile insideFile(inside);
  QFile outsideFile(outside);
  QVERIFY(insideFile.open(QIODevice::WriteOnly));
  QVERIFY(outsideFile.open(QIODevice::WriteOnly));
  insideFile.write("a");
  outsideFile.write("b");
  insideFile.close();
  outsideFile.close();

  QVERIFY(synergy::canonicalFileIsInsideDirectory(root, inside));
  QVERIFY(!synergy::canonicalFileIsInsideDirectory(root, outside));
  QVERIFY(!synergy::canonicalFileIsInsideDirectory(root, root));

  insideFile.remove();
  outsideFile.remove();
  QDir().rmdir(root);
  QDir().rmdir(sibling);
}

QTEST_MAIN(RelativePathTests)
