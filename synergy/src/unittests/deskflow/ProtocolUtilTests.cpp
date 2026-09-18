/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 TuPig
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "ProtocolUtilTests.h"

#include "ProtocolTypes.h"
#include "ProtocolUtil.h"
#include "deskflow/core/DeskflowException.h"
#include "io/IStream.h"

#include <algorithm>
#include <cstring>
#include <deque>

namespace {

class MemoryStream : public deskflow::IStream
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
    // loopback: written bytes become readable, like a connected socket pair
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

} // namespace

void ProtocolUtilTests::initTestCase()
{
  m_log.setFilter(LogLevel::Level::Debug);
}

void ProtocolUtilTests::readBytesAcceptsVariableStringWithinLimit()
{
  MemoryStream stream;
  // encode a variable-length string the way a legitimate peer would
  std::string payload("hello synergy");
  ProtocolUtil::writef(&stream, "%s", &payload);

  std::string result;
  ProtocolUtil::readf(&stream, "%s", &result);

  QCOMPARE(result, payload);
}

void ProtocolUtilTests::readBytesRejectsVariableStringBeyondLimit()
{
  MemoryStream stream;
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
