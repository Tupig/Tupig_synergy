/*
 * Synergy -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 TuPig Synergy Contributors
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "ServerProxyTests.h"

#include "../synergy/MockEventQueue.h"
#include "../synergy/TestStreams.h"

#include "ProtocolTypes.h"
#include "client/IServerProxyClient.h"
#include "client/ServerProxy.h"

#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

namespace {

//! Records what the proxy forwarded, so tests can assert the client contract.
class FakeServerProxyClient : public IServerProxyClient
{
public:
  int disconnectCalls = 0;
  int enterCalls = 0;
  int leaveCalls = 0;
  int mouseMoveCalls = 0;
  int32_t lastX = 0;
  int32_t lastY = 0;

  void disconnect(const char *) override
  {
    ++disconnectCalls;
  }

  void refuseConnection(synergy::core::ConnectionRefusal, const char *) override
  {
  }

  void handshakeComplete() override
  {
  }

  void offerReceivedFiles(const std::vector<std::string> &) override
  {
  }

  size_t getMaximumClipboardReceiveSizeBytes() const override
  {
    return 1024 * 1024;
  }

  void getShape(int32_t &x, int32_t &y, int32_t &width, int32_t &height) const override
  {
    x = 0;
    y = 0;
    width = 1920;
    height = 1080;
  }

  void getCursorPos(int32_t &x, int32_t &y) const override
  {
    x = 0;
    y = 0;
  }

  void enter(int32_t, int32_t, uint32_t, KeyModifierMask, bool) override
  {
    ++enterCalls;
  }

  bool leave() override
  {
    ++leaveCalls;
    return true;
  }

  void setClipboard(ClipboardID, const IClipboard *) override
  {
  }

  void grabClipboard(ClipboardID) override
  {
  }

  void keyDown(KeyID, KeyModifierMask, KeyButton, const std::string &) override
  {
  }

  void keyRepeat(KeyID, KeyModifierMask, int32_t, KeyButton, const std::string &) override
  {
  }

  void keyUp(KeyID, KeyModifierMask, KeyButton) override
  {
  }

  void mouseDown(ButtonID) override
  {
  }

  void mouseUp(ButtonID) override
  {
  }

  void mouseMove(int32_t xAbs, int32_t yAbs) override
  {
    ++mouseMoveCalls;
    lastX = xAbs;
    lastY = yAbs;
  }

  void mouseRelativeMove(int32_t, int32_t) override
  {
  }

  void mouseWheel(int32_t, int32_t) override
  {
  }

  void screensaver(bool) override
  {
  }

  void resetOptions() override
  {
  }

  void setOptions(const OptionsList &) override
  {
  }

  std::string getName() const override
  {
    return "fake-client";
  }
};

//! Exposes the protected parser and its result type for direct testing.
class TestableServerProxy : public ServerProxy
{
public:
  using ServerProxy::ServerProxy;
  using ConnectionResult = ServerProxy::ConnectionResult;
  using ServerProxy::parseMessage;
};

void copyCode(const char *code, uint8_t out[4])
{
  std::memcpy(out, code, 4);
}

} // namespace

void ServerProxyTests::initTestCase()
{
  m_log.setFilter(LogLevel::Level::Error);
}

void ServerProxyTests::unknownMessageReturnsUnknown()
{
  MockEventQueue events;
  FakeServerProxyClient client;
  MemoryStream stream;
  TestableServerProxy proxy(&client, &stream, &events);

  const uint8_t code[4] = {'Z', 'Z', 'Z', 'Z'};
  QCOMPARE(proxy.parseMessage(code), TestableServerProxy::ConnectionResult::Unknown);
}

void ServerProxyTests::serverNoopReturnsOkay()
{
  MockEventQueue events;
  FakeServerProxyClient client;
  MemoryStream stream;
  TestableServerProxy proxy(&client, &stream, &events);

  uint8_t code[4];
  copyCode(kMsgCNoop, code);
  QCOMPARE(proxy.parseMessage(code), TestableServerProxy::ConnectionResult::Okay);
  QCOMPARE(client.disconnectCalls, 0);
}

void ServerProxyTests::serverCloseDisconnects()
{
  MockEventQueue events;
  FakeServerProxyClient client;
  MemoryStream stream;
  TestableServerProxy proxy(&client, &stream, &events);

  uint8_t code[4];
  copyCode(kMsgCClose, code);
  QCOMPARE(proxy.parseMessage(code), TestableServerProxy::ConnectionResult::Disconnect);
  QCOMPARE(client.disconnectCalls, 1);
}

void ServerProxyTests::keepAliveIsEchoed()
{
  MockEventQueue events;
  FakeServerProxyClient client;
  MemoryStream stream(MemoryStream::WriteMode::Loopback);
  TestableServerProxy proxy(&client, &stream, &events);

  uint8_t code[4];
  copyCode(kMsgCKeepAlive, code);
  QCOMPARE(proxy.parseMessage(code), TestableServerProxy::ConnectionResult::Okay);

  // the echo the proxy wrote is visible through the loopback stream
  char buffer[8] = {};
  const uint32_t n = stream.read(buffer, sizeof(buffer));
  QCOMPARE(n, 4u);
  QCOMPARE(std::string(buffer, n), std::string(kMsgCKeepAlive));
}

void ServerProxyTests::mouseMoveForwardsToClient()
{
  MockEventQueue events;
  FakeServerProxyClient client;
  MemoryStream stream;
  TestableServerProxy proxy(&client, &stream, &events);

  // DMMV payload: x=12, y=34 as two 2-byte big-endian ints
  stream.push(std::string("\x00\x0c\x00\x22", 4));

  uint8_t code[4];
  copyCode(kMsgDMouseMove, code);
  QCOMPARE(proxy.parseMessage(code), TestableServerProxy::ConnectionResult::Okay);
  QCOMPARE(client.mouseMoveCalls, 1);
  QCOMPARE(client.lastX, 12);
  QCOMPARE(client.lastY, 34);
}

void ServerProxyTests::enterAndLeaveForwardToClient()
{
  MockEventQueue events;
  FakeServerProxyClient client;
  MemoryStream stream;
  TestableServerProxy proxy(&client, &stream, &events);

  // CINN payload: x=100, y=200, seqNum=7, mask=0
  stream.push(std::string("\x00\x64\x00\xc8", 4) + std::string("\x00\x00\x00\x07", 4) + std::string("\x00\x00", 2));

  uint8_t code[4];
  copyCode(kMsgCEnter, code);
  QCOMPARE(proxy.parseMessage(code), TestableServerProxy::ConnectionResult::Okay);
  QCOMPARE(client.enterCalls, 1);

  // COUT has no payload
  copyCode(kMsgCLeave, code);
  QCOMPARE(proxy.parseMessage(code), TestableServerProxy::ConnectionResult::Okay);
  QCOMPARE(client.leaveCalls, 1);
}

QTEST_MAIN(ServerProxyTests)
