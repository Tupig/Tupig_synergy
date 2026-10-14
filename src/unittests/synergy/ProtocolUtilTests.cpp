/*
 * Synergy -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 TuPig
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "ProtocolUtilTests.h"

#include "OptionTypes.h"
#include "ProtocolTypes.h"
#include "ProtocolUtil.h"
#include "TestStreams.h"
#include "io/IStream.h"
#include "synergy/core/SynergyException.h"

#include <algorithm>
#include <cstring>
#include <deque>

namespace {

} // namespace

void ProtocolUtilTests::initTestCase()
{
  m_log.setFilter(LogLevel::Level::Debug);
}

void ProtocolUtilTests::readBytesAcceptsVariableStringWithinLimit()
{
  MemoryStream stream(MemoryStream::WriteMode::Loopback);
  // encode a variable-length string the way a legitimate peer would
  std::string payload("hello synergy");
  ProtocolUtil::writef(&stream, "%s", &payload);

  std::string result;
  ProtocolUtil::readf(&stream, "%s", &result);

  QCOMPARE(result, payload);
}

void ProtocolUtilTests::readBytesRejectsVariableStringBeyondLimit()
{
  MemoryStream stream(MemoryStream::WriteMode::Loopback);
  // 4 raw bytes, big-endian on the wire: declares a 4GB variable string
  // with no payload following. must be rejected before any allocation.
  stream.push(std::string("\xff\xff\xff\xff", 4));

  std::string result;
  try {
    ProtocolUtil::readf(&stream, "%s", &result);
    QFAIL("expected BadClientException for oversized variable string");
  } catch (const BadClientException &) {
    // expected: peer gets kicked, process stays alive
  }
}

void ProtocolUtilTests::roundTripKeyDownFormat()
{
  // real message body: DKDN%2i%2i%2i (key id, modifier mask, button).
  // writef() takes integer arguments by value (as production callers do);
  // readf() takes pointers, which is where the read side returns the values.
  MemoryStream stream(MemoryStream::WriteMode::Loopback);
  const uint16_t id = 0x1234;
  const uint16_t mask = 0xabcd;
  const uint16_t button = 0x0042;
  ProtocolUtil::writef(&stream, kMsgDKeyDown + 4, id, mask, button);

  uint16_t readId = 0;
  uint16_t readMask = 0;
  uint16_t readButton = 0;
  QVERIFY(ProtocolUtil::readf(&stream, kMsgDKeyDown + 4, &readId, &readMask, &readButton));
  QCOMPARE(readId, id);
  QCOMPARE(readMask, mask);
  QCOMPARE(readButton, button);
}

void ProtocolUtilTests::truncatedKeyDownFormatReturnsFalse()
{
  // three of the six expected bytes: the stream ends mid-message
  MemoryStream stream(MemoryStream::WriteMode::Loopback);
  stream.push(std::string("\x12\x34\xab", 3));

  uint16_t id = 0;
  uint16_t mask = 0;
  uint16_t button = 0;
  QVERIFY(!ProtocolUtil::readf(&stream, kMsgDKeyDown + 4, &id, &mask, &button));
}

void ProtocolUtilTests::roundTripOptionsListFormat()
{
  // real message body: DSOP%4I (count-prefixed list of 4-byte integers)
  MemoryStream stream(MemoryStream::WriteMode::Loopback);
  const OptionsList written = {1, 2, 0xffffffff};
  ProtocolUtil::writef(&stream, kMsgDSetOptions + 4, &written);

  OptionsList read;
  QVERIFY(ProtocolUtil::readf(&stream, kMsgDSetOptions + 4, &read));
  QCOMPARE(read, written);
}

void ProtocolUtilTests::truncatedOptionsListReturnsFalse()
{
  // declares a two-entry list but only carries one entry
  MemoryStream stream(MemoryStream::WriteMode::Loopback);
  stream.push(std::string("\x00\x00\x00\x02\x00\x00\x00\x01", 8));

  OptionsList read;
  QVERIFY(!ProtocolUtil::readf(&stream, kMsgDSetOptions + 4, &read));
}

void ProtocolUtilTests::invalidIntegerLengthThrows()
{
  // %8i is not a valid protocol integer length; the reader must refuse the
  // format instead of reading an arbitrary amount of data
  MemoryStream stream(MemoryStream::WriteMode::Loopback);
  stream.push(std::string("\x00\x00\x00\x00\x00\x00\x00\x01", 8));

  uint32_t value = 0;
  bool threw = false;
  try {
    ProtocolUtil::readf(&stream, "%8i", &value);
  } catch (const BadClientException &) {
    threw = true;
  }
  QVERIFY(threw);
}

QTEST_MAIN(ProtocolUtilTests)
