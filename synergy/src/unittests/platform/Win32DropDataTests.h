/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 TuPig
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "base/Log.h"

#include <QTest>

class Win32DropDataTests : public QObject
{
  Q_OBJECT
private Q_SLOTS:
  void initTestCase();

  // Happy path
  void readsSingleWidePath();
  void readsSeveralWidePaths();
  void preservesNonAsciiPath();
  void readsAnsiPathList();

  // Malformed input: must return nothing rather than read out of bounds
  void rejectsNullBlock();
  void rejectsBlockSmallerThanAHeader();
  void rejectsHeaderOnlyBlock();
  void rejectsZeroFilesOffset();
  void rejectsOffsetAtEndOfBlock();
  void rejectsOffsetBeyondBlock();
  void ignoresUnterminatedTrailingPath();
  void stopsAtTheDoubleNulTerminator();
  void ignoresLeadingEmptyEntry();

  // Bounds
  void capsTheNumberOfPaths();

private:
  //! Build a `CF_HDROP` block the way Windows lays one out.
  /*!
  \param paths the paths to embed
  \param wide write UTF-16 when true, ANSI when false
  \param terminate append the list's terminating NUL (the double-NUL convention)
  \param offsetOverride if non-zero, write this as `pFiles` instead of the real
  offset, to exercise the offset validation
  */
  static std::vector<unsigned char> makeBlock(
      const std::vector<std::wstring> &paths, bool wide = true, bool terminate = true,
      uint32_t offsetOverride = 0
  );

  static std::vector<unsigned char> makeAnsiBlock(
      const std::vector<std::string> &paths, bool terminate = true
  );

  Log m_log;
};
