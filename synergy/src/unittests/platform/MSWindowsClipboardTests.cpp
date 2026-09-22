/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Chris Rizzitello <sithlord48@gmail.com>
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "MSWindowsClipboardTests.h"

#include "MSWindowsClipboard.h"

#include <windows.h>

namespace {
// The Windows clipboard is a single global resource: if another process holds it
// open, or the session blocks access to it, every OpenClipboard() call fails
// with ERROR_ACCESS_DENIED (5) no matter which code performs the call. Probing
// with the raw Win32 API therefore separates "this environment forbids
// clipboard access" from "MSWindowsClipboard is broken", letting the suite skip
// the former instead of reporting a failure that says nothing about this code.
bool clipboardAccessible()
{
  if (OpenClipboard(nullptr) == FALSE) {
    return false;
  }
  CloseClipboard();
  return true;
}

// The clipboard is a shared global resource and can be taken away mid-run by
// another process, so availability has to be re-checked at every open rather
// than once per suite. Expanded inline (rather than wrapped in a function) so
// that QSKIP/QFAIL return from the test function itself; inside a helper they
// would only return from the helper and the test would carry on.
#define REQUIRE_CLIPBOARD_OPEN(clipboard, time)                                                                        \
  do {                                                                                                                 \
    if (!(clipboard).open(time)) {                                                                                     \
      if (!clipboardAccessible()) {                                                                                    \
        QSKIP("the Windows clipboard is unavailable "                                                                  \
              "(held by another process or blocked by the session)");                                                  \
      }                                                                                                                \
      QFAIL("MSWindowsClipboard::open() failed while the clipboard is accessible");                                    \
    }                                                                                                                  \
  } while (false)
} // namespace

void MSWindowsClipboardTests::initTestCase()
{
  m_log.setFilter(LogLevel::Level::Verbose);

  MSWindowsClipboard clipboard(NULL);

  REQUIRE_CLIPBOARD_OPEN(clipboard, 0);
  QVERIFY(clipboard.empty());
}

void MSWindowsClipboardTests::cleanupTestCase()
{
  initTestCase();
}

void MSWindowsClipboardTests::emptyUnusedClipboard()
{
  MSWindowsClipboard clipboard(NULL);
  REQUIRE_CLIPBOARD_OPEN(clipboard, 0);
  QVERIFY(clipboard.emptyUnowned());
}

void MSWindowsClipboardTests::emptyOpenCalled()
{
  MSWindowsClipboard clipboard(NULL);
  REQUIRE_CLIPBOARD_OPEN(clipboard, 0);
  QVERIFY(clipboard.empty());
}

void MSWindowsClipboardTests::emptySingleFormat()
{
  MSWindowsClipboard clipboard(NULL);
  REQUIRE_CLIPBOARD_OPEN(clipboard, 0);

  clipboard.add(IClipboard::Format::Text, m_testString);
  QVERIFY(clipboard.empty());
  QVERIFY(!clipboard.has(IClipboard::Format::Text));
}

void MSWindowsClipboardTests::addValue()
{
  MSWindowsClipboard clipboard(NULL);
  REQUIRE_CLIPBOARD_OPEN(clipboard, 0);

  clipboard.add(IClipboard::Format::Text, m_testString);
  QCOMPARE(clipboard.get(IClipboard::Format::Text), m_testString);
}

void MSWindowsClipboardTests::replaceValue()
{
  using enum IClipboard::Format;

  MSWindowsClipboard clipboard(NULL);
  REQUIRE_CLIPBOARD_OPEN(clipboard, 0);

  clipboard.add(Text, m_testString);
  clipboard.add(Text, m_testString2);

  QCOMPARE(clipboard.get(Text), m_testString2);
}

void MSWindowsClipboardTests::openTimeIsOne()
{
  MSWindowsClipboard clipboard(NULL);
  REQUIRE_CLIPBOARD_OPEN(clipboard, 1);
}

void MSWindowsClipboardTests::closeIsOpen()
{
  MSWindowsClipboard clipboard(NULL);
  REQUIRE_CLIPBOARD_OPEN(clipboard, 1);
  clipboard.close();
}

void MSWindowsClipboardTests::getTimeOpenWithNoEmpty()
{
  MSWindowsClipboard clipboard(NULL);
  REQUIRE_CLIPBOARD_OPEN(clipboard, 1);
  // this behavior is different to that of Clipboard which only
  // returns the value passed into open(t) after empty() is called.
  QCOMPARE(clipboard.getTime(), 1);
}

void MSWindowsClipboardTests::getTimeOpenAndEmpty()
{
  MSWindowsClipboard clipboard(NULL);
  REQUIRE_CLIPBOARD_OPEN(clipboard, 1);
  QVERIFY(clipboard.empty());
  QCOMPARE(clipboard.getTime(), 1);
}

void MSWindowsClipboardTests::has_withFormatAdded()
{
  MSWindowsClipboard clipboard(NULL);
  REQUIRE_CLIPBOARD_OPEN(clipboard, 0);
  QVERIFY(clipboard.empty());

  clipboard.add(IClipboard::Format::Text, m_testString);
  QVERIFY(clipboard.has(IClipboard::Format::Text));
}

void MSWindowsClipboardTests::has_withNoFormatAdded()
{
  MSWindowsClipboard clipboard(NULL);
  REQUIRE_CLIPBOARD_OPEN(clipboard, 0);
  QVERIFY(clipboard.empty());
  QCOMPARE(clipboard.get(IClipboard::Format::Text), "");
}

void MSWindowsClipboardTests::getNonEmptyText()
{
  MSWindowsClipboard clipboard(NULL);
  REQUIRE_CLIPBOARD_OPEN(clipboard, 0);
  QVERIFY(clipboard.empty());

  clipboard.add(IClipboard::Format::Text, m_testString);
  QCOMPARE(clipboard.get(IClipboard::Format::Text), m_testString);
}

void MSWindowsClipboardTests::isOwnedByDeskflow()
{
  MSWindowsClipboard clipboard(NULL);
  REQUIRE_CLIPBOARD_OPEN(clipboard, 0);
  QVERIFY(clipboard.isOwnedByDeskflow());
}

QTEST_MAIN(MSWindowsClipboardTests)
