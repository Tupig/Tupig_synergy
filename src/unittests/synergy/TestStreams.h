/*
 * Synergy -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 TuPig Synergy Contributors
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "io/IStream.h"

#include <algorithm>
#include <cstring>
#include <deque>
#include <string>

//! Shared IStream test doubles.
/*!
These were duplicated across the protocol and file transfer test files. Keeping
one copy here means an IStream interface change only has to be made once.
*/

//! Replays bytes pushed by the test, as the receiving side would read them.
class MemoryStream : public synergy::IStream
{
public:
  //! What write() does with the data.
  enum class WriteMode
  {
    Sink,    //!< discard written bytes
    Loopback //!< make written bytes readable, like a connected socket pair
  };

  explicit MemoryStream(WriteMode mode = WriteMode::Sink) : m_writeMode(mode)
  {
  }

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
    if (m_writeMode == WriteMode::Loopback) {
      m_queue.emplace_back(static_cast<const char *>(buffer), n);
    }
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
  WriteMode m_writeMode = WriteMode::Sink;
  bool m_inputShutdown = false;
};

//! Captures what a sender writes, so the exact wire bytes can be replayed.
class BufferWriteStream : public synergy::IStream
{
public:
  const std::string &str() const
  {
    return m_buffer;
  }

  void close() override
  {
    m_outputShutdown = true;
  }

  uint32_t read(void *, uint32_t) override
  {
    return 0;
  }

  void write(const void *buffer, uint32_t n) override
  {
    if (!m_outputShutdown && n != 0) {
      m_buffer.append(static_cast<const char *>(buffer), n);
    }
  }

  void flush() override
  {
  }

  void shutdownInput() override
  {
  }

  void shutdownOutput() override
  {
    m_outputShutdown = true;
  }

  void *getEventTarget() const override
  {
    return const_cast<BufferWriteStream *>(this);
  }

  bool isReady() const override
  {
    return false;
  }

  uint32_t getSize() const override
  {
    return 0;
  }

private:
  std::string m_buffer;
  bool m_outputShutdown = false;
};

//! A stream that swallows writes, for driving a sender without a network.
class NullWriteStream : public synergy::IStream
{
public:
  void close() override
  {
  }

  uint32_t read(void *, uint32_t) override
  {
    return 0;
  }

  void write(const void *, uint32_t) override
  {
  }

  void flush() override
  {
  }

  void shutdownInput() override
  {
  }

  void shutdownOutput() override
  {
  }

  void *getEventTarget() const override
  {
    return const_cast<NullWriteStream *>(this);
  }

  bool isReady() const override
  {
    return false;
  }

  uint32_t getSize() const override
  {
    return 0;
  }
};
