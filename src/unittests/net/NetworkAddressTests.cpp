/*
 * Synergy -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 TuPig Synergy Contributors
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "NetworkAddressTests.h"

#include "net/NetworkAddress.h"
#include "net/SocketException.h"

#include <functional>
#include <optional>
#include <string>

namespace {

//! Run a constructor that is expected to throw and report the error code.
std::optional<SocketAddressException::SocketError> capturedError(const std::function<void()> &call)
{
  try {
    call();
  } catch (const SocketAddressException &e) {
    return e.getError();
  }
  return std::nullopt;
}

} // namespace

void NetworkAddressTests::parsesHostPortFromSingleColon()
{
  const NetworkAddress address("example.invalid:1234");
  QCOMPARE(address.getHostname(), std::string("example.invalid"));
  QCOMPARE(address.getPort(), 1234);
}

void NetworkAddressTests::parsesBracketedIpv6WithPort()
{
  const NetworkAddress address("[::1]:24800");
  QCOMPARE(address.getHostname(), std::string("::1"));
  QCOMPARE(address.getPort(), 24800);
}

void NetworkAddressTests::parsesUnbracketedLinkLocalWithScopeId()
{
  const NetworkAddress address("fe80::1%lo", 24800);
  QCOMPARE(address.getHostname(), std::string("fe80::1%lo"));
  QCOMPARE(address.getPort(), 24800);
}

void NetworkAddressTests::keepsHostnameWhenNoPortSuffix()
{
  const NetworkAddress address("example.invalid");
  QCOMPARE(address.getHostname(), std::string("example.invalid"));
  QCOMPARE(address.getPort(), 0);
}

void NetworkAddressTests::rejectsEmptyPortAfterColon()
{
  const auto error = capturedError([] { const NetworkAddress address("example.invalid:"); });
  QVERIFY(error.has_value());
  QCOMPARE(*error, SocketAddressException::SocketError::BadPort);
}

void NetworkAddressTests::rejectsNonNumericPort()
{
  const auto error = capturedError([] { const NetworkAddress address("example.invalid:abc"); });
  QVERIFY(error.has_value());
  QCOMPARE(*error, SocketAddressException::SocketError::BadPort);

  const auto ipv6Error = capturedError([] { const NetworkAddress address("[::1]:abc"); });
  QVERIFY(ipv6Error.has_value());
  QCOMPARE(*ipv6Error, SocketAddressException::SocketError::BadPort);
}

void NetworkAddressTests::rejectsPortOutOfRange()
{
  const auto negative = capturedError([] { const NetworkAddress address("example.invalid", -1); });
  QVERIFY(negative.has_value());
  QCOMPARE(*negative, SocketAddressException::SocketError::BadPort);

  const auto tooLarge = capturedError([] { const NetworkAddress address("example.invalid", 65536); });
  QVERIFY(tooLarge.has_value());
  QCOMPARE(*tooLarge, SocketAddressException::SocketError::BadPort);
}

void NetworkAddressTests::rejectsMalformedBracketedIpv6()
{
  const auto error = capturedError([] { const NetworkAddress address("[::1]"); });
  QVERIFY(error.has_value());
  QCOMPARE(*error, SocketAddressException::SocketError::Unknown);
}

void NetworkAddressTests::rejectsLinkLocalWithoutScopeId()
{
  const auto error = capturedError([] { const NetworkAddress address("fe80::1"); });
  QVERIFY(error.has_value());
  QCOMPARE(*error, SocketAddressException::SocketError::Unknown);
}

void NetworkAddressTests::unresolvedAddressIsInvalid()
{
  // the parser must not perform name resolution; an unresolved address has no
  // native address yet
  const NetworkAddress address("example.invalid:1234");
  QVERIFY(!address.isValid());
  QVERIFY(address.getAddress() == nullptr);
}

QTEST_MAIN(NetworkAddressTests)
