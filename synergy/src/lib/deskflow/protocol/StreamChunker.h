/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2013 - 2016 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "ClipboardTypes.h"

#include <string_view>

class IEventQueue;

class StreamChunker
{
public:
  static void sendClipboard(
      const std::string_view &data, size_t size, ClipboardID id, uint32_t sequence, IEventQueue *events,
      void *eventTarget
  );

  //! Payload bytes carried by one clipboard chunk.
  /*!
  Must stay within what the receiver's length-prefixed string reader accepts
  (MessageSizeLimit::ClipboardChunk); a larger chunk is rejected as a protocol
  error and drops the connection.
  */
  static size_t chunkSize();
};
