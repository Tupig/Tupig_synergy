/*
 * Synergy -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 TuPig Synergy Contributors
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "ClipboardTypes.h"
#include "KeyTypes.h"
#include "MouseTypes.h"
#include "OptionTypes.h"
#include "common/Enums.h"

#include <cstdint>
#include <string>
#include <vector>

class IClipboard;

//! The subset of the client that the server proxy drives.
/*!
ServerProxy depends only on this interface, so tests can substitute a fake
without constructing a full Client (with its sockets, screens and event
sources). Client implements the interface alongside IClient.
*/
class IServerProxyClient
{
public:
  virtual ~IServerProxyClient() = default;

  virtual void disconnect(const char *msg) = 0;
  virtual void refuseConnection(synergy::core::ConnectionRefusal reason, const char *msg) = 0;
  virtual void handshakeComplete() = 0;
  virtual void offerReceivedFiles(const std::vector<std::string> &paths) = 0;
  virtual size_t getMaximumClipboardReceiveSizeBytes() const = 0;

  virtual void getShape(int32_t &x, int32_t &y, int32_t &width, int32_t &height) const = 0;
  virtual void getCursorPos(int32_t &x, int32_t &y) const = 0;

  virtual void enter(int32_t xAbs, int32_t yAbs, uint32_t seqNum, KeyModifierMask mask, bool forScreensaver) = 0;
  virtual bool leave() = 0;
  virtual void setClipboard(ClipboardID, const IClipboard *) = 0;
  virtual void grabClipboard(ClipboardID) = 0;
  virtual void keyDown(KeyID, KeyModifierMask, KeyButton, const std::string &) = 0;
  virtual void keyRepeat(KeyID, KeyModifierMask, int32_t count, KeyButton, const std::string &lang) = 0;
  virtual void keyUp(KeyID, KeyModifierMask, KeyButton) = 0;
  virtual void mouseDown(ButtonID) = 0;
  virtual void mouseUp(ButtonID) = 0;
  virtual void mouseMove(int32_t xAbs, int32_t yAbs) = 0;
  virtual void mouseRelativeMove(int32_t xRel, int32_t yRel) = 0;
  virtual void mouseWheel(int32_t xDelta, int32_t yDelta) = 0;
  virtual void screensaver(bool activate) = 0;
  virtual void resetOptions() = 0;
  virtual void setOptions(const OptionsList &options) = 0;
  virtual std::string getName() const = 0;
};
