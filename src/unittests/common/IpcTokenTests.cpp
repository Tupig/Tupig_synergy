/*
 * Synergy -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 TuPig Synergy Contributors
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "IpcTokenTests.h"

#include "common/IpcToken.h"
#include "common/Settings.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>

#ifdef Q_OS_WIN
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>
#include <aclapi.h>
#include <sddl.h>

#include <string>
#endif

void IpcTokenTests::initTestCase()
{
  // per-process settings directory so the token file is never written into the
  // machine's real configuration
  m_testDir = QDir::tempPath() + QString("/synergy-ipc-token-tests-%1").arg(QCoreApplication::applicationPid());
  QDir().mkpath(m_testDir);
  Settings::setSettingsFile(m_testDir + "/Synergy.conf");
  Settings::setStateFile(m_testDir + "/Synergy.state");
}

void IpcTokenTests::cleanupTestCase()
{
  QDir(m_testDir).removeRecursively();
}

void IpcTokenTests::generateProduces64LowercaseHex()
{
  const auto first = synergy::generateIpcToken();
  QCOMPARE(first.size(), 64);

  static const QRegularExpression hex(QStringLiteral("^[0-9a-f]{64}$"));
  QVERIFY(hex.match(first).hasMatch());

  // tokens must not repeat
  QVERIFY(first != synergy::generateIpcToken());
}

void IpcTokenTests::writeRejectsEmptyToken()
{
  QVERIFY(!synergy::writeIpcToken(m_serverName, QString()));
}

void IpcTokenTests::readMissingFileReturnsEmpty()
{
  QVERIFY(synergy::readIpcToken(QStringLiteral("no-such-server-name")).isEmpty());
}

void IpcTokenTests::writeThenReadRoundTrips()
{
  const auto token = synergy::generateIpcToken();
  QVERIFY(synergy::writeIpcToken(m_serverName, token));
  QCOMPARE(synergy::readIpcToken(m_serverName), token);
}

void IpcTokenTests::writeReplacesExistingToken()
{
  const auto first = synergy::generateIpcToken();
  const auto second = synergy::generateIpcToken();
  QVERIFY(synergy::writeIpcToken(m_serverName, first));
  QVERIFY(synergy::writeIpcToken(m_serverName, second));
  QCOMPARE(synergy::readIpcToken(m_serverName), second);
}

void IpcTokenTests::tokenPathIsUnderSettingsPath()
{
  const auto path = synergy::ipcTokenPath(m_serverName);
  QCOMPARE(QFileInfo(path).fileName(), m_serverName + QStringLiteral(".ipc-token"));

  const auto actualDir = QDir::fromNativeSeparators(QFileInfo(path).absolutePath());
  const auto expectedDir = QDir::cleanPath(QDir::fromNativeSeparators(Settings::settingsPath()));
  QCOMPARE(actualDir, expectedDir);
}

#ifndef Q_OS_WIN
void IpcTokenTests::writeSetsOwnerOnlyPermissions()
{
  const auto token = synergy::generateIpcToken();
  QVERIFY(synergy::writeIpcToken(m_serverName, token));

  QCOMPARE(QFile::permissions(synergy::ipcTokenPath(m_serverName)), QFileDevice::ReadOwner | QFileDevice::WriteOwner);
}

void IpcTokenTests::refreshRestoresOwnerOnlyPermissions()
{
  const auto token = synergy::generateIpcToken();
  QVERIFY(synergy::writeIpcToken(m_serverName, token));

  const auto path = synergy::ipcTokenPath(m_serverName);
  QVERIFY(
      QFile::setPermissions(
          path, QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ReadGroup | QFileDevice::WriteGroup
      )
  );

  QVERIFY(synergy::refreshIpcTokenAccess(m_serverName));
  QCOMPARE(QFile::permissions(path), QFileDevice::ReadOwner | QFileDevice::WriteOwner);
}
#else
void IpcTokenTests::writeAppliesRestrictedDacl()
{
  const auto token = synergy::generateIpcToken();
  QVERIFY(synergy::writeIpcToken(m_serverName, token));

  const auto path = synergy::ipcTokenPath(m_serverName);
  const std::wstring nativePath = path.toStdWString();

  PACL dacl = nullptr;
  PSECURITY_DESCRIPTOR descriptor = nullptr;
  QCOMPARE(
      GetNamedSecurityInfoW(
          const_cast<LPWSTR>(nativePath.c_str()), SE_FILE_OBJECT, DACL_SECURITY_INFORMATION, nullptr, nullptr, &dacl,
          nullptr, &descriptor
      ),
      static_cast<DWORD>(ERROR_SUCCESS)
  );

  LPWSTR sddl = nullptr;
  QVERIFY(
      ConvertSecurityDescriptorToStringSecurityDescriptorW(
          descriptor, SDDL_REVISION_1, DACL_SECURITY_INFORMATION, &sddl, nullptr
      ) == TRUE
  );
  const std::wstring text(sddl);
  LocalFree(sddl);
  LocalFree(descriptor);

  QVERIFY(text.find(L";;;SY)") != std::wstring::npos);
  QVERIFY(text.find(L";;;BA)") != std::wstring::npos);
  // interactive users must never get access
  QVERIFY(text.find(L";;;IU)") == std::wstring::npos);
}
#endif

QTEST_MAIN(IpcTokenTests)
