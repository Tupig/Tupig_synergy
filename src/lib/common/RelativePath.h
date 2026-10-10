/*
 * Synergy -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 TuPig
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <QDir>
#include <QFileInfo>
#include <QString>

namespace synergy {

//! True when \p path is a relative path with no parent-directory segment.
/*!
Absolute paths, drive letters, UNC paths, and `.` / `..` segments are rejected.
The daemon and screen commands accept only this shape, then join it to a known
directory. An absolute path from IPC would let another local user point the
service at a file they chose.
*/
inline bool isSafeRelativePath(const QString &path)
{
  constexpr int kMaxRelativePathLength = 512;
  if (path.isEmpty() || path.size() > kMaxRelativePathLength) {
    return false;
  }

  if (path.contains(QLatin1Char('"')) || path.contains(QLatin1Char('\n')) || path.contains(QLatin1Char('\r'))) {
    return false;
  }

  if (path.startsWith(QLatin1String("\\\\")) || path.startsWith(QLatin1String("//"))) {
    return false;
  }
  if (path.startsWith(QLatin1Char('/')) || path.startsWith(QLatin1Char('\\'))) {
    return false;
  }
  if (path.size() >= 2 && path.at(1) == QLatin1Char(':')) {
    return false;
  }

  const auto normalized = QDir::fromNativeSeparators(path);
  if (normalized.startsWith(QLatin1Char('/'))) {
    return false;
  }

  const auto parts = normalized.split(QLatin1Char('/'), Qt::SkipEmptyParts);
  if (parts.isEmpty()) {
    return false;
  }
  for (const auto &part : parts) {
    if (part == QLatin1String(".") || part == QLatin1String("..")) {
      return false;
    }
  }
  return true;
}

//! Path of \p absoluteFile relative to \p directory, or empty when it is not safe.
inline QString relativeToDirectory(const QString &directory, const QString &absoluteFile)
{
  auto relative = QDir::fromNativeSeparators(
      QDir(directory).relativeFilePath(QFileInfo(absoluteFile).absoluteFilePath())
  );
  if (relative.startsWith(QLatin1String("./")))
    relative.remove(0, 2);
  if (!isSafeRelativePath(relative)) {
    return {};
  }
  return relative;
}

//! Join a checked relative path onto \p directory. Empty when \p relative is not safe.
inline QString resolveRelativePath(const QString &directory, const QString &relative)
{
  if (!isSafeRelativePath(relative)) {
    return {};
  }
  return QDir(directory).filePath(QDir::fromNativeSeparators(relative));
}

//! True when \p filePath is an existing file whose canonical path stays inside \p directory.
/*!
Symlinks and junctions are resolved before the check, so a link inside the
settings directory that points outside it is rejected. The comparison is
case-insensitive only on Windows.
*/
inline bool canonicalFileIsInsideDirectory(const QString &directory, const QString &filePath)
{
  const QFileInfo fileInfo(filePath);
  if (!fileInfo.isFile()) {
    return false;
  }

  const auto file = QDir::fromNativeSeparators(fileInfo.canonicalFilePath());
  const auto root = QDir::fromNativeSeparators(QFileInfo(directory).canonicalFilePath());
  if (file.isEmpty() || root.isEmpty()) {
    return false;
  }

#ifdef Q_OS_WIN
  const auto sensitivity = Qt::CaseInsensitive;
#else
  const auto sensitivity = Qt::CaseSensitive;
#endif
  return file.startsWith(root + QLatin1Char('/'), sensitivity);
}

} // namespace synergy
