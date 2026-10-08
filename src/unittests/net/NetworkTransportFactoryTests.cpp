/*
 * Synergy -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 TuPig
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "NetworkTransportFactoryTests.h"

#include "net/NetworkTransportFactory.h"
#include "net/QtNetworkTransport.h"

// The factory pins, for the upcoming default-switch work, that:
//  * the default transport stays Legacy until the wiring task flips it,
//  * USE_LEGACY_NETWORK=1/0 overrides the configured type,
//  * every creation path returns owned (unique_ptr) transports,
//  * a Legacy configuration without a socket factory fails closed (nullptr)
//    instead of crashing.

void NetworkTransportFactoryTests::init()
{
  qunsetenv("USE_LEGACY_NETWORK");
}

void NetworkTransportFactoryTests::cleanup()
{
  qunsetenv("USE_LEGACY_NETWORK");
}

void NetworkTransportFactoryTests::defaultTypeIsLegacy()
{
  NetworkTransportFactory factory;
  QCOMPARE(static_cast<int>(factory.getTransportType()), static_cast<int>(TransportType::Legacy));
}

void NetworkTransportFactoryTests::envForcesLegacy()
{
  qputenv("USE_LEGACY_NETWORK", "1");
  NetworkTransportFactory factory(TransportType::Qt);
  QCOMPARE(static_cast<int>(factory.getTransportType()), static_cast<int>(TransportType::Legacy));
}

void NetworkTransportFactoryTests::envForcesQt()
{
  qputenv("USE_LEGACY_NETWORK", "0");
  NetworkTransportFactory factory(TransportType::Legacy);
  QCOMPARE(static_cast<int>(factory.getTransportType()), static_cast<int>(TransportType::Qt));
}

void NetworkTransportFactoryTests::qtCreationPaths()
{
  NetworkTransportFactory factory(TransportType::Qt);

  auto transport = factory.createTransport(IArchNetwork::AddressFamily::INet, SecurityLevel::PlainText);
  QVERIFY(transport != nullptr);
  QCOMPARE(static_cast<int>(transport->getSecurityLevel()), static_cast<int>(SecurityLevel::PlainText));

  auto listen = factory.createListenTransport(IArchNetwork::AddressFamily::INet, SecurityLevel::Encrypted);
  QVERIFY(listen != nullptr);
}

void NetworkTransportFactoryTests::legacyWithoutFactoryReturnsNull()
{
  NetworkTransportFactory factory(TransportType::Legacy, nullptr, nullptr);
  QVERIFY(factory.createTransport() == nullptr);
  QVERIFY(factory.createListenTransport() == nullptr);
}

void NetworkTransportFactoryTests::qtIsAvailable()
{
  QVERIFY(NetworkTransportFactory::isQtTransportAvailable());
}

QTEST_MAIN(NetworkTransportFactoryTests)
