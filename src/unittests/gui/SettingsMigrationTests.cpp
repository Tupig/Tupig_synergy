/*
 * Synergy -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Synergy App Ltd
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 *
 * This package is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * found in the file LICENSE that should have accompanied this file.
 *
 * This package is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "SettingsMigrationTests.h"

#include "common/Settings.h"
#include "synergy/gui/SettingsMigrationMap.h"

#include <QSettings>
#include <QTemporaryDir>

using synergy::gui::migration::looksLikeLegacySettings;
using synergy::gui::migration::mapLegacyKey;

void SettingsMigrationTests::mapsKnownLegacyKeys()
{
  const auto screenName = mapLegacyKey("screenName", QStringLiteral("desk"));
  QVERIFY(screenName.has_value());
  QCOMPARE(screenName->first, Settings::Core::ComputerName);
  QCOMPARE(screenName->second.toString(), QStringLiteral("desk"));

  const auto port = mapLegacyKey("port", 24800);
  QVERIFY(port.has_value());
  QCOMPARE(port->first, Settings::Core::Port);
  QCOMPARE(port->second.toInt(), 24800);

  const auto host = mapLegacyKey("serverHostname", QStringLiteral("192.168.1.2"));
  QVERIFY(host.has_value());
  QCOMPARE(host->first, Settings::Client::RemoteHost);

  const auto crypto = mapLegacyKey("cryptoEnabled", true);
  QVERIFY(crypto.has_value());
  QCOMPARE(crypto->first, Settings::Security::TlsEnabled);
  QCOMPARE(crypto->second.toBool(), true);
}

void SettingsMigrationTests::mapsCoreModeFromCheckedGroups()
{
  const auto server = mapLegacyKey("groupServerChecked", true);
  QVERIFY(server.has_value());
  QCOMPARE(server->first, Settings::Core::CoreMode);
  QCOMPARE(server->second.toInt(), static_cast<int>(Settings::Server));

  const auto client = mapLegacyKey("groupClientChecked", true);
  QVERIFY(client.has_value());
  QCOMPARE(client->second.toInt(), static_cast<int>(Settings::Client));

  // an unchecked group carries no information and is dropped
  QVERIFY(!mapLegacyKey("groupServerChecked", false).has_value());
  QVERIFY(!mapLegacyKey("groupClientChecked", false).has_value());
}

void SettingsMigrationTests::mapsProcessModeFromEnableService()
{
  const auto service = mapLegacyKey("enableService", true);
  QVERIFY(service.has_value());
  QCOMPARE(service->first, Settings::Core::ProcessMode);
  QCOMPARE(service->second.toInt(), static_cast<int>(Settings::Service));

  const auto desktop = mapLegacyKey("enableService", false);
  QVERIFY(desktop.has_value());
  QCOMPARE(desktop->second.toInt(), static_cast<int>(Settings::Desktop));
}

void SettingsMigrationTests::dropsUnknownKeys()
{
  QVERIFY(!mapLegacyKey("someObsoleteKey", 1).has_value());
  QVERIFY(!mapLegacyKey(QString(), 1).has_value());
}

void SettingsMigrationTests::detectsLegacyStoresBySentinelKeys()
{
  QTemporaryDir dir;
  QVERIFY(dir.isValid());

  QSettings plain(dir.filePath(QStringLiteral("plain.conf")), QSettings::IniFormat);
  plain.setValue(QStringLiteral("unrelated/key"), 1);
  plain.sync();
  QVERIFY(!looksLikeLegacySettings(plain));

  QSettings legacy(dir.filePath(QStringLiteral("legacy.conf")), QSettings::IniFormat);
  legacy.setValue(QStringLiteral("startedBefore"), true);
  legacy.sync();
  QVERIFY(looksLikeLegacySettings(legacy));

  QSettings legacyScope(dir.filePath(QStringLiteral("scope.conf")), QSettings::IniFormat);
  legacyScope.setValue(QStringLiteral("systemScope"), true);
  legacyScope.sync();
  QVERIFY(looksLikeLegacySettings(legacyScope));
}

QTEST_MAIN(SettingsMigrationTests)
