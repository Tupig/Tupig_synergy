/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 TuPig
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "Win32DropDataTests.h"

#include "Win32DropData.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <ShlObj.h>

#include <cstring>
#include <string>
#include <vector>

namespace {

constexpr size_t kCap = 4096;

void appendWide(std::vector<unsigned char> &out, const std::wstring &text)
{
  for (wchar_t unit : text) {
    unsigned char bytes[sizeof(wchar_t)];
    std::memcpy(bytes, &unit, sizeof(wchar_t));
    out.insert(out.end(), bytes, bytes + sizeof(wchar_t));
  }
}

void appendWideNul(std::vector<unsigned char> &out)
{
  appendWide(out, std::wstring(1, L'\0'));
}

} // namespace

std::vector<unsigned char> Win32DropDataTests::makeBlock(
    const std::vector<std::wstring> &paths, bool wide, bool terminate, uint32_t offsetOverride
)
{
  // This helper only builds UTF-16 lists. ANSI coverage goes through makeAnsiBlock
  // so the conversion path is not accidentally skipped.
  Q_ASSERT(wide);

  std::vector<unsigned char> list;
  for (const auto &path : paths) {
    appendWide(list, path);
    appendWideNul(list);
  }
  if (terminate) {
    appendWideNul(list);
  }

  DROPFILES header{};
  header.pFiles = sizeof(DROPFILES);
  header.fWide = TRUE;
  if (offsetOverride != 0) {
    header.pFiles = offsetOverride;
  }

  std::vector<unsigned char> block(sizeof(DROPFILES) + list.size());
  std::memcpy(block.data(), &header, sizeof(DROPFILES));
  if (!list.empty()) {
    std::memcpy(block.data() + sizeof(DROPFILES), list.data(), list.size());
  }
  return block;
}

std::vector<unsigned char> Win32DropDataTests::makeAnsiBlock(const std::vector<std::string> &paths, bool terminate)
{
  std::vector<unsigned char> list;
  for (const auto &path : paths) {
    list.insert(list.end(), path.begin(), path.end());
    list.push_back('\0');
  }
  if (terminate) {
    list.push_back('\0');
  }

  DROPFILES header{};
  header.pFiles = sizeof(DROPFILES);
  header.fWide = FALSE;

  std::vector<unsigned char> block(sizeof(DROPFILES) + list.size());
  std::memcpy(block.data(), &header, sizeof(DROPFILES));
  if (!list.empty()) {
    std::memcpy(block.data() + sizeof(DROPFILES), list.data(), list.size());
  }
  return block;
}

void Win32DropDataTests::initTestCase()
{
  m_log.setFilter(LogLevel::Level::Verbose);
}

void Win32DropDataTests::readsSingleWidePath()
{
  const auto block = makeBlock({L"C:\\Temp\\a.txt"});
  const auto paths = deskflow::win32::readDropFilePaths(block.data(), block.size());
  QCOMPARE(paths.size(), size_t{1});
  QCOMPARE(QString::fromStdString(paths[0]), QStringLiteral("C:\\Temp\\a.txt"));
}

void Win32DropDataTests::readsSeveralWidePaths()
{
  const auto block = makeBlock({L"C:\\a.txt", L"D:\\b\\c.bin", L"E:\\d"});
  const auto paths = deskflow::win32::readDropFilePaths(block.data(), block.size());
  QCOMPARE(paths.size(), size_t{3});
  QCOMPARE(QString::fromStdString(paths[0]), QStringLiteral("C:\\a.txt"));
  QCOMPARE(QString::fromStdString(paths[1]), QStringLiteral("D:\\b\\c.bin"));
  QCOMPARE(QString::fromStdString(paths[2]), QStringLiteral("E:\\d"));
}

void Win32DropDataTests::preservesNonAsciiPath()
{
  // The old wcstombs path mangled anything outside the active code page.
  const auto block = makeBlock({L"C:\\Temp\\照片.png"});
  const auto paths = deskflow::win32::readDropFilePaths(block.data(), block.size());
  QCOMPARE(paths.size(), size_t{1});
  QCOMPARE(QString::fromStdString(paths[0]), QStringLiteral("C:\\Temp\\照片.png"));
}

void Win32DropDataTests::readsAnsiPathList()
{
  const auto block = makeAnsiBlock({"C:\\Temp\\ansi.txt", "D:\\other.dat"});
  const auto paths = deskflow::win32::readDropFilePaths(block.data(), block.size());
  QCOMPARE(paths.size(), size_t{2});
  QCOMPARE(QString::fromStdString(paths[0]), QStringLiteral("C:\\Temp\\ansi.txt"));
  QCOMPARE(QString::fromStdString(paths[1]), QStringLiteral("D:\\other.dat"));
}

void Win32DropDataTests::rejectsNullBlock()
{
  QVERIFY(deskflow::win32::readDropFilePaths(nullptr, 64).empty());
}

void Win32DropDataTests::rejectsBlockSmallerThanAHeader()
{
  unsigned char tiny[sizeof(DROPFILES) - 1] = {};
  QVERIFY(deskflow::win32::readDropFilePaths(tiny, sizeof(tiny)).empty());
}

void Win32DropDataTests::rejectsHeaderOnlyBlock()
{
  // A header with pFiles pointing at sizeof(DROPFILES) but no list bytes left.
  const auto block = makeBlock({}, true, false);
  QVERIFY(deskflow::win32::readDropFilePaths(block.data(), block.size()).empty());
}

void Win32DropDataTests::rejectsZeroFilesOffset()
{
  const auto block = makeBlock({L"C:\\x"}, true, true, /*offsetOverride=*/0);
  // makeBlock only overrides when non-zero; force a zero offset by rewriting.
  auto mutated = block;
  DROPFILES header{};
  std::memcpy(&header, mutated.data(), sizeof(DROPFILES));
  header.pFiles = 0;
  std::memcpy(mutated.data(), &header, sizeof(DROPFILES));
  QVERIFY(deskflow::win32::readDropFilePaths(mutated.data(), mutated.size()).empty());
}

void Win32DropDataTests::rejectsOffsetAtEndOfBlock()
{
  auto block = makeBlock({L"C:\\x"});
  DROPFILES header{};
  std::memcpy(&header, block.data(), sizeof(DROPFILES));
  header.pFiles = static_cast<DWORD>(block.size());
  std::memcpy(block.data(), &header, sizeof(DROPFILES));
  QVERIFY(deskflow::win32::readDropFilePaths(block.data(), block.size()).empty());
}

void Win32DropDataTests::rejectsOffsetBeyondBlock()
{
  const auto block = makeBlock({L"C:\\x"}, true, true, /*offsetOverride=*/0x7fffffff);
  QVERIFY(deskflow::win32::readDropFilePaths(block.data(), block.size()).empty());
}

void Win32DropDataTests::ignoresUnterminatedTrailingPath()
{
  // Path bytes present but no terminating NUL: safer to drop than guess.
  auto block = makeBlock({L"C:\\Temp\\cut"}, true, /*terminate=*/false);
  // Remove the path's own trailing NUL so the string is truly unterminated.
  // makeBlock always writes a NUL after each path; strip the last two bytes
  // (one wchar_t NUL) so only the path characters remain.
  QVERIFY(block.size() >= sizeof(DROPFILES) + sizeof(wchar_t));
  block.resize(block.size() - sizeof(wchar_t));
  QVERIFY(deskflow::win32::readDropFilePaths(block.data(), block.size()).empty());
}

void Win32DropDataTests::stopsAtTheDoubleNulTerminator()
{
  // Extra garbage after the list terminator must not become a path.
  auto block = makeBlock({L"C:\\only.txt"});
  const unsigned char junk[] = {'X', 'Y', 'Z', 0, 0};
  block.insert(block.end(), junk, junk + sizeof(junk));
  const auto paths = deskflow::win32::readDropFilePaths(block.data(), block.size());
  QCOMPARE(paths.size(), size_t{1});
  QCOMPARE(QString::fromStdString(paths[0]), QStringLiteral("C:\\only.txt"));
}

void Win32DropDataTests::ignoresLeadingEmptyEntry()
{
  // An empty first entry is the list terminator, so nothing is returned.
  std::vector<unsigned char> list;
  appendWideNul(list);
  appendWide(list, L"C:\\should-not-appear.txt");
  appendWideNul(list);
  appendWideNul(list);

  DROPFILES header{};
  header.pFiles = sizeof(DROPFILES);
  header.fWide = TRUE;

  std::vector<unsigned char> block(sizeof(DROPFILES) + list.size());
  std::memcpy(block.data(), &header, sizeof(DROPFILES));
  std::memcpy(block.data() + sizeof(DROPFILES), list.data(), list.size());

  QVERIFY(deskflow::win32::readDropFilePaths(block.data(), block.size()).empty());
}

void Win32DropDataTests::capsTheNumberOfPaths()
{
  std::vector<std::wstring> many;
  many.reserve(kCap + 8);
  for (size_t i = 0; i < kCap + 8; ++i) {
    many.push_back(L"C:\\f" + std::to_wstring(i) + L".txt");
  }
  const auto block = makeBlock(many);
  const auto paths = deskflow::win32::readDropFilePaths(block.data(), block.size());
  QCOMPARE(paths.size(), kCap);
}

void Win32DropDataTests::buildDropFileBlockRoundTrips()
{
  const std::vector<std::string> original = {
      "C:\\Users\\test\\a.txt",
      "D:\\文档\\file.bin",
  };
  const auto block = deskflow::win32::buildDropFileBlock(original);
  QVERIFY(!block.empty());
  const auto paths = deskflow::win32::readDropFilePaths(block.data(), block.size());
  QCOMPARE(paths.size(), original.size());
  QCOMPARE(QString::fromStdString(paths[0]), QString::fromStdString(original[0]));
  QCOMPARE(QString::fromStdString(paths[1]), QString::fromStdString(original[1]));
}

void Win32DropDataTests::buildDropFileBlockRejectsEmpty()
{
  QVERIFY(deskflow::win32::buildDropFileBlock({}).empty());
}

QTEST_MAIN(Win32DropDataTests)
#include "Win32DropDataTests.moc"
