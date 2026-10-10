/*
 * Synergy -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Chris Rizzitello <https://github.com/Tupig/Tupig_synergy/issues>
 * SPDX-FileCopyrightText: (C) 2014 - 2021 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "StringTests.h"

#include "base/String.h"

void StringTests::formatWithArgs()
{
  const char *format = "%%%{1}=%{2}";
  const char *arg1 = "answer";
  const char *arg2 = "42";

  std::string result = synergy::string::format(format, arg1, arg2);

  QCOMPARE(result, "%answer=42");
}

void StringTests::formatedString()
{
  const char *format = "%s=%d";
  const char *arg1 = "answer";
  int arg2 = 42;

  std::string result = synergy::string::sprintf(format, arg1, arg2);

  QCOMPARE("answer=42", result);
}

void StringTests::sprintfKeepsStringThatFillsTheFirstBuffer()
{
  const std::string payload(1024, 'a');
  const auto result = synergy::string::sprintf("%s", payload.c_str());
  QCOMPARE(result, payload);
}

QTEST_MAIN(StringTests)
