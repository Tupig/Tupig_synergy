/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 TuPig
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "Win32DropData.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
// DROPFILES lives here; shellapi.h alone is not enough under WIN32_LEAN_AND_MEAN.
#include <ShlObj.h>

#include <cstring>
#include <string_view>

namespace {

//! Upper bound on the number of paths taken from one block.
/*!
A drop of thousands of files is not something to act on in this feature (the
receiving side caps the count anyway), and the cap keeps a malformed block from
turning into an unbounded amount of work.
*/
constexpr size_t kMaxDropPaths = 4096;

std::string wideToUtf8(std::wstring_view text)
{
  if (text.empty()) {
    return {};
  }

  const auto length = static_cast<int>(text.size());
  const int needed = ::WideCharToMultiByte(CP_UTF8, 0, text.data(), length, nullptr, 0, nullptr, nullptr);
  if (needed <= 0) {
    return {};
  }

  std::string out(static_cast<size_t>(needed), '\0');
  const int written =
      ::WideCharToMultiByte(CP_UTF8, 0, text.data(), length, out.data(), needed, nullptr, nullptr);
  if (written <= 0) {
    return {};
  }

  out.resize(static_cast<size_t>(written));
  return out;
}

std::string ansiToUtf8(std::string_view text)
{
  if (text.empty()) {
    return {};
  }

  const auto length = static_cast<int>(text.size());
  const int needed = ::MultiByteToWideChar(CP_ACP, 0, text.data(), length, nullptr, 0);
  if (needed <= 0) {
    return {};
  }

  std::wstring wide(static_cast<size_t>(needed), L'\0');
  const int written = ::MultiByteToWideChar(CP_ACP, 0, text.data(), length, wide.data(), needed);
  if (written <= 0) {
    return {};
  }
  wide.resize(static_cast<size_t>(written));

  return wideToUtf8(wide);
}

//! Walk a double-NUL terminated list of UTF-16 paths.
void collectWide(const unsigned char *list, size_t listSize, std::vector<std::string> &paths)
{
  const size_t units = listSize / sizeof(wchar_t);
  std::wstring current;

  for (size_t i = 0; i < units && paths.size() < kMaxDropPaths; ++i) {
    // Read element-wise through memcpy: the block is only guaranteed to be
    // suitably aligned for a byte view, and this keeps the walk defined.
    wchar_t unit = L'\0';
    std::memcpy(&unit, list + (i * sizeof(wchar_t)), sizeof(wchar_t));

    if (unit == L'\0') {
      if (current.empty()) {
        // An empty entry is the terminator, not a path. The list ends here.
        break;
      }
      paths.push_back(wideToUtf8(current));
      current.clear();
    } else {
      current.push_back(unit);
    }
  }

  // A trailing path with no terminator is not acted on: the block is short, so
  // the path may be truncated and a truncated path is worse than none.
}

//! Walk a double-NUL terminated list of ANSI paths.
void collectAnsi(const unsigned char *list, size_t listSize, std::vector<std::string> &paths)
{
  std::string current;

  for (size_t i = 0; i < listSize && paths.size() < kMaxDropPaths; ++i) {
    const char unit = static_cast<char>(list[i]);

    if (unit == '\0') {
      if (current.empty()) {
        break;
      }
      paths.push_back(ansiToUtf8(current));
      current.clear();
    } else {
      current.push_back(unit);
    }
  }
}

} // namespace

std::vector<std::string> deskflow::win32::readDropFilePaths(const void *data, std::size_t size)
{
  std::vector<std::string> paths;

  if (data == nullptr || size < sizeof(DROPFILES)) {
    return paths;
  }

  const auto *bytes = static_cast<const unsigned char *>(data);

  DROPFILES header{};
  std::memcpy(&header, bytes, sizeof(DROPFILES));

  // pFiles is an offset from the start of this block. Reject anything that is
  // not inside it, including an unset (zero) value, rather than trusting the
  // sender's arithmetic.
  if (header.pFiles < sizeof(DROPFILES) || header.pFiles >= size) {
    return paths;
  }

  const auto *list = bytes + header.pFiles;
  const size_t listSize = size - header.pFiles;

  if (header.fWide) {
    collectWide(list, listSize, paths);
  } else {
    collectAnsi(list, listSize, paths);
  }

  return paths;
}

namespace {

std::wstring utf8ToWide(std::string_view text)
{
  if (text.empty()) {
    return {};
  }

  const auto length = static_cast<int>(text.size());
  const int needed = ::MultiByteToWideChar(CP_UTF8, 0, text.data(), length, nullptr, 0);
  if (needed <= 0) {
    return {};
  }

  std::wstring out(static_cast<size_t>(needed), L'\0');
  const int written = ::MultiByteToWideChar(CP_UTF8, 0, text.data(), length, out.data(), needed);
  if (written <= 0) {
    return {};
  }
  out.resize(static_cast<size_t>(written));
  return out;
}

} // namespace

std::vector<unsigned char> deskflow::win32::buildDropFileBlock(const std::vector<std::string> &utf8Paths)
{
  if (utf8Paths.empty() || utf8Paths.size() > kMaxDropPaths) {
    return {};
  }

  std::vector<wchar_t> list;
  for (const auto &path : utf8Paths) {
    const std::wstring wide = utf8ToWide(path);
    if (wide.empty() && !path.empty()) {
      return {};
    }
    list.insert(list.end(), wide.begin(), wide.end());
    list.push_back(L'\0');
  }
  list.push_back(L'\0');

  const size_t listBytes = list.size() * sizeof(wchar_t);
  std::vector<unsigned char> block(sizeof(DROPFILES) + listBytes);

  DROPFILES header{};
  header.pFiles = sizeof(DROPFILES);
  header.fWide = TRUE;
  std::memcpy(block.data(), &header, sizeof(DROPFILES));
  std::memcpy(block.data() + sizeof(DROPFILES), list.data(), listBytes);
  return block;
}

HGLOBAL deskflow::win32::createDropFilesHGlobal(const std::vector<std::string> &utf8Paths)
{
  const auto block = buildDropFileBlock(utf8Paths);
  if (block.empty()) {
    return nullptr;
  }

  HGLOBAL handle = ::GlobalAlloc(GHND, block.size());
  if (handle == nullptr) {
    return nullptr;
  }

  void *locked = ::GlobalLock(handle);
  if (locked == nullptr) {
    ::GlobalFree(handle);
    return nullptr;
  }

  std::memcpy(locked, block.data(), block.size());
  ::GlobalUnlock(handle);
  return handle;
}
