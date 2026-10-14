/*
 * Synergy -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 TuPig
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "PacketStreamFilterTests.h"

#include "PacketStreamFilter.h"
#include "ProtocolTypes.h"
#include "io/IStream.h"

#include "MockEventQueue.h"

#include <algorithm>
#include <cstring>
#include <deque>
#include <string>

namespace {

class MemoryStream : public synergy::IStream
{
public:
  void push(const std::string &bytes)
  {
    m_queue.push_back(bytes);
  }

  void close() override
  {
    m_queue.clear();
    m_inputShutdown = true;
  }

  uint32_t read(void *buffer, uint32_t n) override
  {
    if (m_inputShutdown || m_queue.empty() || n == 0) {
      return 0;
    }

    auto &front = m_queue.front();
    const size_t take = std::min(static_cast<size_t>(n), front.size());
    if (buffer != nullptr) {
      std::memcpy(buffer, front.data(), take);
    }

    front.erase(0, take);
    if (front.empty()) {
      m_queue.pop_front();
    }

    return static_cast<uint32_t>(take);
  }

  void write(const void *buffer, uint32_t n) override
  {
    m_queue.emplace_back(static_cast<const char *>(buffer), n);
  }

  void flush() override
  {
  }

  void shutdownInput() override
  {
    close();
  }

  void shutdownOutput() override
  {
  }

  void *getEventTarget() const override
  {
    return const_cast<MemoryStream *>(this);
  }

  bool isReady() const override
  {
    return !m_inputShutdown && !m_queue.empty();
  }

  uint32_t getSize() const override
  {
    size_t total = 0;
    for (const auto &chunk : m_queue) {
      total += chunk.size();
    }
    return static_cast<uint32_t>(std::min<size_t>(total, UINT32_MAX));
  }

private:
  std::deque<std::string> m_queue;
  bool m_inputShutdown = false;
};

//! Exposes the protected event hook so tests can simulate input-ready.
class TestableFilter : public PacketStreamFilter
{
public:
  using PacketStreamFilter::filterEvent;
  using PacketStreamFilter::PacketStreamFilter;
};

std::string makeFrameHeader(uint32_t size)
{
  std::string header(4, '\0');
  header[0] = static_cast<char>((size >> 24) & 0xff);
  header[1] = static_cast<char>((size >> 16) & 0xff);
  header[2] = static_cast<char>((size >> 8) & 0xff);
  header[3] = static_cast<char>(size & 0xff);
  return header;
}

void feedInputReady(TestableFilter &filter)
{
  filter.filterEvent(Event(EventTypes::StreamInputReady, filter.getEventTarget()));
}

} // namespace

void PacketStreamFilterTests::initTestCase()
{
  m_log.setFilter(LogLevel::Level::Debug);
}

void PacketStreamFilterTests::splitFrame_reassemblesPayloadAcrossChunks()
{
  MemoryStream stream;
  MockEventQueue events;
  TestableFilter filter(&events, &stream, false);

  const std::string payload("hello");
  stream.push(makeFrameHeader(static_cast<uint32_t>(payload.size())).substr(0, 3));
  feedInputReady(filter);
  QVERIFY(!filter.isReady());

  stream.push(makeFrameHeader(static_cast<uint32_t>(payload.size())).substr(3) + payload);
  feedInputReady(filter);
  QVERIFY(filter.isReady());

  char buffer[16] = {};
  QCOMPARE(filter.read(buffer, sizeof(buffer)), static_cast<uint32_t>(payload.size()));
  QCOMPARE(std::string(buffer, payload.size()), payload);
}

void PacketStreamFilterTests::oversizedHeader_marksFormatErrorAndDropsStream()
{
  MemoryStream stream;
  MockEventQueue events;
  TestableFilter filter(&events, &stream, false);

  // 4 raw bytes on the wire: declares a 4GB packet, larger than any
  // legitimate peer would send. must be rejected before buffering it.
  stream.push(std::string("\xff\xff\xff\xff", 4));
  feedInputReady(filter);

  QVERIFY(!filter.isReady());
  char buffer[8] = {};
  QCOMPARE(filter.read(buffer, sizeof(buffer)), 0u);
}

void PacketStreamFilterTests::declaredLengthBeyondData_staysNotReady()
{
  MemoryStream stream;
  MockEventQueue events;
  TestableFilter filter(&events, &stream, false);

  // header claims 8 bytes of payload but only 4 follow; the frame is
  // incomplete and must never be delivered
  stream.push(makeFrameHeader(8) + std::string(4, 'x'));
  feedInputReady(filter);

  QVERIFY(!filter.isReady());
  char buffer[8] = {};
  QCOMPARE(filter.read(buffer, sizeof(buffer)), 0u);
}

void PacketStreamFilterTests::write_prependsFourByteLengthPrefix()
{
  MemoryStream stream;
  MockEventQueue events;
  TestableFilter filter(&events, &stream, false);

  filter.write("abcd", 4);

  char buffer[16] = {};
  const uint32_t n = stream.read(buffer, sizeof(buffer));
  QCOMPARE(n, 8u);

  const std::string framed(buffer, n);
  QCOMPARE(framed.substr(0, 4), makeFrameHeader(4));
  QCOMPARE(framed.substr(4), std::string("abcd"));
}
QTEST_MAIN(PacketStreamFilterTests)
