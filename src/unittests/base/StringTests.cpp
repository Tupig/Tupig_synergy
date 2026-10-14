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

void StringTests::linefeedsNormalizeMixedInputToLf()
{
  using synergy::string::convertLinefeedsToLf;

  QCOMPARE(convertLinefeedsToLf("a\r\nb"), std::string("a\nb"));
  QCOMPARE(convertLinefeedsToLf("a\rb"), std::string("a\nb"));
  QCOMPARE(convertLinefeedsToLf("a\nb"), std::string("a\nb"));
  QCOMPARE(convertLinefeedsToLf("x\r\ny\rz\n"), std::string("x\ny\nz\n"));

  // no line endings: identity
  QCOMPARE(convertLinefeedsToLf("plain"), std::string("plain"));
  QCOMPARE(convertLinefeedsToLf(""), std::string(""));
}

void StringTests::linefeedsExpandToPlatformTerminator()
{
  using synergy::string::convertLinefeedsFromLf;

  // Windows terminator
  QCOMPARE(convertLinefeedsFromLf("a\nb", "\r\n"), std::string("a\r\nb"));

  // classic Mac terminator
  QCOMPARE(convertLinefeedsFromLf("a\nb", "\r"), std::string("a\rb"));

  // no line endings: identity
  QCOMPARE(convertLinefeedsFromLf("plain", "\r\n"), std::string("plain"));
}

void StringTests::linefeedsRoundTripAcrossPlatforms()
{
  // the canonical form plus an expansion must return the original text for
  // both platform conventions, including mixed input from a broken peer
  const std::string mixed = "one\r\ntwo\rthree\nfour";
  const auto canonical = synergy::string::convertLinefeedsToLf(mixed);
  QCOMPARE(canonical, std::string("one\ntwo\nthree\nfour"));

  QCOMPARE(synergy::string::convertLinefeedsFromLf(canonical, "\r\n"), std::string("one\r\ntwo\r\nthree\r\nfour"));
  QCOMPARE(synergy::string::convertLinefeedsFromLf(canonical, "\r"), std::string("one\rtwo\rthree\rfour"));
}

QTEST_MAIN(StringTests)
