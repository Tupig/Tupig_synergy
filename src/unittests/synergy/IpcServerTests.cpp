/*
 * Synergy -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 TuPig Synergy Contributors
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "IpcServerTests.h"

#include "common/IpcToken.h"
#include "common/Settings.h"
#include "common/VersionInfo.h"
#include "ipc/IpcServer.h"

#include <QCoreApplication>
#include <QDir>
#include <QLocalServer>
#include <QLocalSocket>

#include <memory>

namespace {

class TestIpcServer : public synergy::core::ipc::IpcServer
{
public:
  using IpcServer::IpcServer;

  QStringList commands;

protected:
  void processCommand(QLocalSocket *, const QString &command, const QStringList &) override
  {
    commands.append(command);
  }
};

std::unique_ptr<QLocalSocket> openClient(const QString &serverName)
{
  auto client = std::make_unique<QLocalSocket>();
  client->connectToServer(serverName);
  if (!client->waitForConnected(5000)) {
    return nullptr;
  }
  return client;
}

const QString kErrorHelloRequired = QStringLiteral("error=hello required\n");

} // namespace

void IpcServerTests::initTestCase()
{
  // per-process settings directory so tokens are written to a scratch path
  m_testDir = QDir::tempPath() + QString("/synergy-ipc-server-tests-%1").arg(QCoreApplication::applicationPid());
  QDir().mkpath(m_testDir);
  Settings::setSettingsFile(m_testDir + "/Synergy.conf");
  Settings::setStateFile(m_testDir + "/Synergy.state");

  m_serverName = QStringLiteral("synergy-ipc-server-tests-%1").arg(QCoreApplication::applicationPid());
}

void IpcServerTests::cleanupTestCase()
{
  QLocalServer::removeServer(m_serverName);
  QDir(m_testDir).removeRecursively();
}

void IpcServerTests::commandBeforeHelloRejected()
{
  TestIpcServer server(nullptr, m_serverName, QStringLiteral("test"));
  QVERIFY(server.listen());

  auto client = openClient(m_serverName);
  QVERIFY(client != nullptr);

  client->write("start\n");
  client->flush();

  QTRY_VERIFY(client->state() == QLocalSocket::UnconnectedState);
  QCOMPARE(QString::fromUtf8(client->readAll()), kErrorHelloRequired);
  QVERIFY(server.commands.isEmpty());
}

void IpcServerTests::helloWithoutTokenRejected()
{
  TestIpcServer server(nullptr, m_serverName, QStringLiteral("test"));
  QVERIFY(server.listen());

  auto client = openClient(m_serverName);
  QVERIFY(client != nullptr);

  client->write("hello=some-version\n");
  client->flush();

  QTRY_VERIFY(client->state() == QLocalSocket::UnconnectedState);
  QCOMPARE(QString::fromUtf8(client->readAll()), kErrorHelloRequired);
}

void IpcServerTests::helloWithWrongTokenRejected()
{
  TestIpcServer server(nullptr, m_serverName, QStringLiteral("test"));
  QVERIFY(server.listen());

  auto client = openClient(m_serverName);
  QVERIFY(client != nullptr);

  client->write("hello=some-version wrong-token\n");
  client->flush();

  QTRY_VERIFY(client->state() == QLocalSocket::UnconnectedState);
  QCOMPARE(QString::fromUtf8(client->readAll()), kErrorHelloRequired);
}

void IpcServerTests::helloWithWrongVersionReportsMismatch()
{
  TestIpcServer server(nullptr, m_serverName, QStringLiteral("test"));
  QVERIFY(server.listen());

  const auto token = synergy::readIpcToken(m_serverName);
  QVERIFY(!token.isEmpty());

  auto client = openClient(m_serverName);
  QVERIFY(client != nullptr);

  client->write(QStringLiteral("hello=not-the-server-version %1\n").arg(token).toUtf8());
  client->flush();

  QTRY_VERIFY(client->bytesAvailable() > 0);
  QCOMPARE(
      QString::fromUtf8(client->readAll()), QStringLiteral("versionMismatch=%1\n").arg(QString::fromUtf8(kVersion))
  );
}

void IpcServerTests::validHelloAcceptsCommands()
{
  TestIpcServer server(nullptr, m_serverName, QStringLiteral("test"));
  QVERIFY(server.listen());

  const auto token = synergy::readIpcToken(m_serverName);
  QVERIFY(!token.isEmpty());

  auto client = openClient(m_serverName);
  QVERIFY(client != nullptr);

  const auto version = QString::fromUtf8(kVersion);
  client->write(QStringLiteral("hello=%1 %2\n").arg(version, token).toUtf8());
  client->flush();

  QTRY_VERIFY(client->bytesAvailable() > 0);
  QCOMPARE(QString::fromUtf8(client->readAll()), QStringLiteral("hello=%1\n").arg(version));

  client->write("start\n");
  client->flush();

  QTRY_COMPARE(server.commands.size(), 1);
  QCOMPARE(server.commands.at(0), QStringLiteral("start"));
}

void IpcServerTests::oversizedMessageDisconnects()
{
  TestIpcServer server(nullptr, m_serverName, QStringLiteral("test"));
  QVERIFY(server.listen());

  auto client = openClient(m_serverName);
  QVERIFY(client != nullptr);

  // no newline, so this accumulates in the server's read buffer until it
  // crosses the 64 KiB message limit
  client->write(QByteArray(70 * 1024, 'a'));
  client->flush();

  QTRY_VERIFY(client->state() == QLocalSocket::UnconnectedState);
}

QTEST_MAIN(IpcServerTests)
