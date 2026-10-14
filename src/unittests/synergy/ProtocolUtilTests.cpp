/*
 * Synergy -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 TuPig
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "ProtocolUtilTests.h"

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

QTEST_MAIN(ProtocolUtilTests)
