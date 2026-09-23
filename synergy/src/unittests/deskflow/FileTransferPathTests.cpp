/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 TuPig
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "FileTransferPathTests.h"

#include "deskflow/protocol/FileTransferPath.h"

#include <string>
#include <string_view>
#include <vector>

namespace {
const std::string kSafe = "holiday-photo.jpg";
}

void FileTransferPathTests::initTestCase()
{
  m_log.setFilter(LogLevel::Level::Verbose);
}

void FileTransferPathTests::acceptsOrdinaryName()
{
  QCOMPARE(FileTransferPath::sanitizeFileName(kSafe), kSafe);
}

void FileTransferPathTests::acceptsUnicodeName()
{
  // Names travel as UTF-8 bytes; non-ASCII is not itself suspicious.
  const std::string name = "照片-2026.png";
  QCOMPARE(FileTransferPath::sanitizeFileName(name), name);
}

void FileTransferPathTests::rejectsEmptyName()
{
  QCOMPARE(FileTransferPath::sanitizeFileName(""), std::string());
}

void FileTransferPathTests::rejectsParentDirectoryReferences()
{
  QCOMPARE(FileTransferPath::sanitizeFileName("."), std::string());
  QCOMPARE(FileTransferPath::sanitizeFileName(".."), std::string());
}

void FileTransferPathTests::rejectsUnixPathTraversal()
{
  // The classic traversal payloads. Each must be refused outright rather than
  // stripped down to a harmless-looking basename, so the attempt stays visible.
  QCOMPARE(FileTransferPath::sanitizeFileName("../../etc/passwd"), std::string());
  QCOMPARE(FileTransferPath::sanitizeFileName("../secret"), std::string());
  QCOMPARE(FileTransferPath::sanitizeFileName("sub/dir/file.txt"), std::string());
  QCOMPARE(FileTransferPath::sanitizeFileName("trailing/"), std::string());
}

void FileTransferPathTests::rejectsWindowsPathTraversal()
{
  QCOMPARE(FileTransferPath::sanitizeFileName("..\\..\\Windows\\System32\\evil.dll"), std::string());
  QCOMPARE(FileTransferPath::sanitizeFileName("sub\\dir\\file.txt"), std::string());
}

void FileTransferPathTests::rejectsAbsolutePosixPath()
{
  QCOMPARE(FileTransferPath::sanitizeFileName("/etc/shadow"), std::string());
  QCOMPARE(FileTransferPath::sanitizeFileName("/tmp/x"), std::string());
}

void FileTransferPathTests::rejectsDriveQualifiedPath()
{
  QCOMPARE(FileTransferPath::sanitizeFileName("C:\\Windows\\evil.exe"), std::string());
  QCOMPARE(FileTransferPath::sanitizeFileName("C:evil.exe"), std::string());
  // UNC paths carry separators, which are refused on their own.
  QCOMPARE(FileTransferPath::sanitizeFileName("\\\\server\\share\\x"), std::string());
}

void FileTransferPathTests::rejectsControlCharacters()
{
  QCOMPARE(FileTransferPath::sanitizeFileName("new\nline"), std::string());
  QCOMPARE(FileTransferPath::sanitizeFileName("tab\there"), std::string());
  QCOMPARE(FileTransferPath::sanitizeFileName("bell\x07"), std::string());
  QCOMPARE(FileTransferPath::sanitizeFileName("del\x7f"), std::string());
}

void FileTransferPathTests::rejectsNulEmbeddedName()
{
  // A NUL would truncate the name at the C API boundary, so "safe.txt\0.exe"
  // must not be allowed to look like either name.
  const std::string withNul("safe.txt\0.exe", 13);
  QCOMPARE(FileTransferPath::sanitizeFileName(withNul), std::string());
}

void FileTransferPathTests::rejectsWindowsReservedDeviceNames()
{
  for (const auto &name : {"CON", "con", "PRN", "AUX", "NUL", "COM1", "COM9", "LPT1", "LPT9"}) {
    QCOMPARE(FileTransferPath::sanitizeFileName(name), std::string());
  }
}

void FileTransferPathTests::rejectsReservedDeviceNameWithExtension()
{
  // Windows treats con.txt as the CON device, so the stem is what matters.
  QCOMPARE(FileTransferPath::sanitizeFileName("con.txt"), std::string());
  QCOMPARE(FileTransferPath::sanitizeFileName("NUL.log"), std::string());
  QCOMPARE(FileTransferPath::sanitizeFileName("com1.dat"), std::string());
}

void FileTransferPathTests::rejectsTrailingDotOrSpace()
{
  // Windows strips these, so "evil." and "evil" would collide after the fact.
  QCOMPARE(FileTransferPath::sanitizeFileName("evil."), std::string());
  QCOMPARE(FileTransferPath::sanitizeFileName("evil "), std::string());
  QCOMPARE(FileTransferPath::sanitizeFileName(".. "), std::string());
}

void FileTransferPathTests::rejectsWindowsForbiddenCharacters()
{
  for (const auto &name : {"a<b", "a>b", "a:b", "a\"b", "a|b", "a?b", "a*b"}) {
    QCOMPARE(FileTransferPath::sanitizeFileName(name), std::string());
  }
}

void FileTransferPathTests::rejectsOverlongName()
{
  QCOMPARE(FileTransferPath::sanitizeFileName(std::string(256, 'a')), std::string());
  // Exactly at the limit is still fine.
  const std::string atLimit(255, 'a');
  QCOMPARE(FileTransferPath::sanitizeFileName(atLimit), atLimit);
}

void FileTransferPathTests::acceptsNameThatMerelyContainsDots()
{
  // ".." inside a name is not traversal; only a whole segment is.
  const std::string name = "archive..tar.gz";
  QCOMPARE(FileTransferPath::sanitizeFileName(name), name);

  const std::string dotted = "..hidden";
  QCOMPARE(FileTransferPath::sanitizeFileName(dotted), dotted);
}

void FileTransferPathTests::joinUnderDirectoryAddsSeparatorOnce()
{
  QCOMPARE(FileTransferPath::joinUnderDirectory("/tmp/drop", kSafe), std::string("/tmp/drop/") + kSafe);
  QCOMPARE(FileTransferPath::joinUnderDirectory("/tmp/drop/", kSafe), std::string("/tmp/drop/") + kSafe);
  QCOMPARE(FileTransferPath::joinUnderDirectory("", kSafe), kSafe);
}

void FileTransferPathTests::joinNamesRoundTripsThroughSplit()
{
  const std::vector<std::string> names = {"a.txt", "photo.png", "照片.jpg"};
  const auto payload = FileTransferPath::joinNames(names);

  // Each name is NUL-terminated, including the last.
  QVERIFY(payload.size() > 0);
  QCOMPARE(payload.back(), '\0');
  QCOMPARE(FileTransferPath::splitNames(payload), names);
}

void FileTransferPathTests::splitNamesDropsEmptySegments()
{
  // A stray empty segment is never something to act on.
  QCOMPARE(FileTransferPath::splitNames(std::string_view{}), std::vector<std::string>{});
  QCOMPARE(FileTransferPath::splitNames(std::string_view("\0", 1)), std::vector<std::string>{});
  QCOMPARE(
      FileTransferPath::splitNames(std::string_view("a\0\0b\0", 5)), (std::vector<std::string>{"a", "b"})
  );
}

QTEST_MAIN(FileTransferPathTests)
