/*
 * Synergy -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Chris Rizzitello <sithlord48@gmail.com>
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2011 Nick Bolton
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "ClipboardTests.h"

#include "Clipboard.h"

void ClipboardTests::initTestCase()
{
  m_log.setFilter(LogLevel::Level::Verbose);
}

void ClipboardTests::basicFunction()
{
  Clipboard clipboard;

  std::string actual = clipboard.marshall();
  // seems to return "\0\0\0\0" but EXPECT_EQ can't assert this,
  // so instead, just assert that first char is '\0'.
  QCOMPARE((int)actual[0], 0);

  QVERIFY(clipboard.open(0));
  QVERIFY(clipboard.empty());
  QCOMPARE(clipboard.getTime(), 0);

  clipboard.close();

  QVERIFY(clipboard.open(1));
  QCOMPARE(clipboard.getTime(), 0);
}

void ClipboardTests::basicText()
{
  using enum IClipboard::Format;

  Clipboard clipboard;
  QVERIFY(clipboard.open(0));
  QVERIFY(!clipboard.has(Text));
  QCOMPARE(clipboard.get(Text), "");

  clipboard.add(Text, kTestString1);
  QVERIFY(clipboard.has(Text));
  QCOMPARE(clipboard.get(Text), kTestString1);

  std::string actual = clipboard.marshall();
  // string contains other data, but 8th char should be kText.
  QCOMPARE(static_cast<char>(Text), actual.at(7));
  QCOMPARE((int)actual[11], kTestString1.length());

  // // marshall closes the clipboard
  QVERIFY(clipboard.open(0));
  QVERIFY(clipboard.empty());

  clipboard.add(Text, kTestString2);
  QCOMPARE(clipboard.get(Text), kTestString2);
  clipboard.close();
}

void ClipboardTests::longerText()
{
  std::string text;
  text.append("Synergy is Free and Open Source Software that lets you ");
  text.append("easily share your mouse and keyboard between multiple ");
  text.append("computers, where each computer has it's own display. No ");
  text.append("special hardware is required, all you need is a local area ");
  text.append("network. Synergy is supported on Windows, Mac OS X and Linux.");

  Clipboard clipboard;
  clipboard.open(0);
  clipboard.add(IClipboard::Format::Text, text);
  clipboard.close();

  std::string actual = clipboard.marshall();

  // the 32-bit payload size is stored big-endian in the 4 chars at [8..11].
  // Derive the expected bytes from the actual text size so the test does not
  // need updating every time the sample text changes (a previous hardcoded
  // 287 broke as soon as the text was edited).
  const auto size = static_cast<uint32_t>(text.size());
  QCOMPARE(static_cast<uint8_t>(actual[8]), static_cast<uint8_t>(size >> 24));
  QCOMPARE(static_cast<uint8_t>(actual[9]), static_cast<uint8_t>(size >> 16));
  QCOMPARE(static_cast<uint8_t>(actual[10]), static_cast<uint8_t>(size >> 8));
  QCOMPARE(static_cast<uint8_t>(actual[11]), static_cast<uint8_t>(size));
}

void ClipboardTests::htmlText()
{
  Clipboard clipboard;
  clipboard.open(0);
  clipboard.add(IClipboard::Format::HTML, kTestString1);
  clipboard.close();

  std::string actual = clipboard.marshall();

  // string contains other data, but 8th char should be kHTML.
  QCOMPARE(static_cast<int>(IClipboard::Format::HTML), static_cast<int>(actual.at(7)));
}

void ClipboardTests::dualText()
{
  Clipboard clipboard;
  clipboard.open(0);
  clipboard.add(IClipboard::Format::Text, kTestString1);
  clipboard.add(IClipboard::Format::HTML, kTestString2);
  clipboard.close();

  std::string actual = clipboard.marshall();

  // the number of formats is stored inside the first 4 chars.
  // the writeUInt32 function right-aligns numbers in 4 chars,
  // so if you right align 2, it will be "\0\0\0\2" in a string.
  // we assert that the char at the 4th index is 2 (the number of
  // formats that we've added).
  QCOMPARE((int)actual[3], 2);
}

void ClipboardTests::marshalText()
{
  Clipboard clipboard;
  clipboard.open(0);
  clipboard.add(IClipboard::Format::Text, kTestString1);
  clipboard.close();

  std::string actual = clipboard.marshall();
  // string contains other data, but should end in the string we added.
  QCOMPARE(actual.substr(12), kTestString1);
}

void ClipboardTests::unMarshalText()
{
  Clipboard clipboard;
  std::string data;
  data += (char)0;
  data += (char)0;
  data += (char)0;
  data += (char)0; // 0 formats added
  clipboard.unmarshall(data, 0);
  clipboard.open(0);

  QVERIFY(!clipboard.has(IClipboard::Format::Text));
  clipboard.close();
}

void ClipboardTests::unMarshalLongerText()
{
  Clipboard clipboard;

  std::string text;
  text.append("Synergy is Free and Open Source Software that lets you ");
  text.append("easily share your mouse and keyboard between multiple ");
  text.append("computers, where each computer has it's own display. No ");
  text.append("special hardware is required, all you need is a local area ");
  text.append("network. Synergy is supported on Windows, Mac OS X and Linux.");

  std::string data;
  data += (char)0;
  data += (char)0;
  data += (char)0;
  data += (char)1; // 1 format added
  data += (char)0;
  data += (char)0;
  data += (char)0;
  data += (char)IClipboard::Format::Text;
  const auto size = static_cast<uint32_t>(text.size());
  data += (char)(size >> 24);
  data += (char)(size >> 16);
  data += (char)(size >> 8);
  data += (char)(size);
  data += text;

  clipboard.unmarshall(data, 0);
  clipboard.open(0);
  QCOMPARE(clipboard.get(IClipboard::Format::Text), text);
  clipboard.close();
}

void ClipboardTests::unMarshalTextAndHtml()
{
  Clipboard clipboard;
  std::string data;
  data += (char)0;
  data += (char)0;
  data += (char)0;
  data += (char)2; // 2 formats added
  data += (char)0;
  data += (char)0;
  data += (char)0;
  data += (char)IClipboard::Format::Text;
  data += (char)0;
  data += (char)0;
  data += (char)0;
  data += static_cast<char>(kTestString1.size());
  data += kTestString1;
  data += (char)0;
  data += (char)0;
  data += (char)0;
  data += (char)IClipboard::Format::HTML;
  data += (char)0;
  data += (char)0;
  data += (char)0;
  data += static_cast<char>(kTestString2.size());
  data += kTestString2;

  clipboard.unmarshall(data, 0);
  clipboard.open(0);
  QCOMPARE(clipboard.get(IClipboard::Format::Text), kTestString1);
  QCOMPARE(clipboard.get(IClipboard::Format::HTML), kTestString2);
  clipboard.close();
}

void ClipboardTests::equalClipboards()
{
  Clipboard clipboard1;
  clipboard1.open(0);
  clipboard1.add(IClipboard::Format::Text, kTestString1);
  clipboard1.close();

  Clipboard clipboard2;
  Clipboard::copy(&clipboard2, &clipboard1);

  clipboard2.open(0);
  QCOMPARE(clipboard2.get(IClipboard::Format::Text), kTestString1);
  clipboard2.close();
}

QTEST_MAIN(ClipboardTests)
