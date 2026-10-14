/*
 * Synergy -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 TuPig Synergy Contributors
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "ClientProxyTests.h"

#include "../synergy/MockEventQueue.h"

#include "ProtocolTypes.h"
#include "io/IStream.h"
#include "server/ClientProxy1_0.h"

#include <algorithm>
#include <cstring>
#include <deque>
#include <initializer_list>
#include <string>

namespace {

//! In-memory stream whose input queue is independent from written bytes.
class FakeStream : public synergy::IStream
{
public:
  void push(const std::string &bytes)
  {
    m_queue.push_back(bytes);
  }

  void close() override
  {
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
    m_written.append(static_cast<const char *>(buffer), n);
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
    return const_cast<FakeStream *>(this);
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
  std::string m_written;
  bool m_inputShutdown = false;
};

//! Exposes the protected message parsers so tests can drive them directly.
class TestableClientProxy1_0 : public ClientProxy1_0
{
public:
  using ClientProxy1_0::ClientProxy1_0;
  using ClientProxy1_0::parseHandshakeMessage;
  using ClientProxy1_0::parseMessage;
};

void copyCode(const char *code, uint8_t out[4])
{
  std::memcpy(out, code, 4);
}

//! The DINF payload (without the message code): seven 2-byte big-endian ints.
std::string infoPayload(std::initializer_list<int> values)
{
  std::string bytes;
  for (const int value : values) {
    bytes += static_cast<char>((value >> 8) & 0xff);
    bytes += static_cast<char>(value & 0xff);
  }
  return bytes;
}

} // namespace

void ClientProxyTests::initTestCase()
{
  m_log.setFilter(LogLevel::Level::Error);
}

void ClientProxyTests::handshakeNoopReturnsTrue()
{
  MockEventQueue events;
  // the proxy adopts the stream and deletes it on destruction
  auto *stream = new FakeStream();
  TestableClientProxy1_0 proxy("test-client", stream, &events);

  uint8_t code[4];
  copyCode(kMsgCNoop, code);
  QVERIFY(proxy.parseHandshakeMessage(code));
}

void ClientProxyTests::handshakeUnknownCodeReturnsFalse()
{
  MockEventQueue events;
  auto *stream = new FakeStream();
  TestableClientProxy1_0 proxy("test-client", stream, &events);

  const uint8_t code[4] = {'Z', 'Z', 'Z', 'Z'};
  QVERIFY(!proxy.parseHandshakeMessage(code));
}

void ClientProxyTests::handshakeInfoCompletesHandshake()
{
  MockEventQueue events;
  auto *stream = new FakeStream();
  TestableClientProxy1_0 proxy("test-client", stream, &events);

  // the payload parsed by recvInfo (format without the leading code)
  stream->push(infoPayload({10, 20, 800, 600, 0, 5, 6}));

  uint8_t code[4];
  copyCode(kMsgDInfo, code);
  QVERIFY(proxy.parseHandshakeMessage(code));
}

void ClientProxyTests::parseMessageNoopReturnsTrue()
{
  MockEventQueue events;
  auto *stream = new FakeStream();
  TestableClientProxy1_0 proxy("test-client", stream, &events);

  uint8_t code[4];
  copyCode(kMsgCNoop, code);
  QVERIFY(proxy.parseMessage(code));
}

void ClientProxyTests::parseMessageUnknownReturnsFalse()
{
  MockEventQueue events;
  auto *stream = new FakeStream();
  TestableClientProxy1_0 proxy("test-client", stream, &events);

  const uint8_t code[4] = {'Z', 'Z', 'Z', 'Z'};
  QVERIFY(!proxy.parseMessage(code));
}

void ClientProxyTests::parseMessageInfoUpdatesShape()
{
  MockEventQueue events;
  auto *stream = new FakeStream();
  TestableClientProxy1_0 proxy("test-client", stream, &events);

  stream->push(infoPayload({10, 20, 800, 600, 0, 5, 6}));

  uint8_t code[4];
  copyCode(kMsgDInfo, code);
  QVERIFY(proxy.parseMessage(code));

  int32_t x = 0;
  int32_t y = 0;
  int32_t w = 0;
  int32_t h = 0;
  proxy.getShape(x, y, w, h);
  QCOMPARE(x, 10);
  QCOMPARE(y, 20);
  QCOMPARE(w, 800);
  QCOMPARE(h, 600);
}

void ClientProxyTests::parseMessageTruncatedInfoReturnsFalse()
{
  MockEventQueue events;
  auto *stream = new FakeStream();
  TestableClientProxy1_0 proxy("test-client", stream, &events);

  // the stream does not contain the payload the parser expects
  uint8_t code[4];
  copyCode(kMsgDInfo, code);
  QVERIFY(!proxy.parseMessage(code));
}

QTEST_MAIN(ClientProxyTests)
