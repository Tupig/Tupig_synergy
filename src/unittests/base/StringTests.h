/*
 * Synergy -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Chris Rizzitello <https://github.com/Tupig/Tupig_synergy/issues>
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include <QTest>

class StringTests : public QObject
{
  Q_OBJECT
private Q_SLOTS:
  void formatWithArgs();
  void formatedString();
  void sprintfKeepsStringThatFillsTheFirstBuffer();
};
