/*
 * Synergy -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 TuPig
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "QSettingsProxyTests.h"

#include "common/QSettingsProxy.h"

#include <QTemporaryDir>

void QSettingsProxyTests::roundTripValues()
{
  QTemporaryDir dir;
  QVERIFY(dir.isValid());
  const auto path = dir.filePath(QStringLiteral("proxy.conf"));

  QSettingsProxy proxy;
  proxy.load(path);
  QVERIFY(proxy.isWritable());

  proxy.setValue(QStringLiteral("group/key"), 42);
  proxy.setValue(QStringLiteral("flag"), true);
  proxy.sync();

  QVERIFY(proxy.contains(QStringLiteral("group/key")));
  QCOMPARE(proxy.value(QStringLiteral("group/key")).toInt(), 42);
  QCOMPARE(proxy.value(QStringLiteral("missing"), QStringLiteral("fallback")).toString(), QStringLiteral("fallback"));

  // a second proxy sees the persisted data
  QSettingsProxy second;
  second.load(path);
  QCOMPARE(second.value(QStringLiteral("group/key")).toInt(), 42);
  QCOMPARE(second.value(QStringLiteral("flag")).toBool(), true);
}

void QSettingsProxyTests::groupsAndArrays()
{
  QTemporaryDir dir;
  QVERIFY(dir.isValid());
  const auto path = dir.filePath(QStringLiteral("proxy.conf"));

  QSettingsProxy proxy;
  proxy.load(path);

  proxy.beginGroup(QStringLiteral("screen"));
  proxy.setValue(QStringLiteral("name"), QStringLiteral("desk"));
  proxy.endGroup();
  QCOMPARE(proxy.value(QStringLiteral("screen/name")).toString(), QStringLiteral("desk"));

  proxy.beginWriteArray(QStringLiteral("clients"));
  proxy.setArrayIndex(0);
  proxy.setValue(QStringLiteral("name"), QStringLiteral("a"));
  proxy.setArrayIndex(1);
  proxy.setValue(QStringLiteral("name"), QStringLiteral("b"));
  proxy.endArray();
  proxy.sync();

  QCOMPARE(proxy.beginReadArray(QStringLiteral("clients")), 2);
  proxy.setArrayIndex(1);
  QCOMPARE(proxy.value(QStringLiteral("name")).toString(), QStringLiteral("b"));
  proxy.endArray();

  proxy.remove(QStringLiteral("screen/name"));
  QVERIFY(!proxy.contains(QStringLiteral("screen/name")));
}

void QSettingsProxyTests::clearRemovesEverything()
{
  QTemporaryDir dir;
  QVERIFY(dir.isValid());
  const auto path = dir.filePath(QStringLiteral("proxy.conf"));

  QSettingsProxy proxy;
  proxy.load(path);
  proxy.setValue(QStringLiteral("a/b"), 1);
  proxy.setValue(QStringLiteral("c"), 2);
  proxy.sync();

  proxy.clear();
  proxy.sync();
  QVERIFY(!proxy.contains(QStringLiteral("a/b")));
  QVERIFY(!proxy.contains(QStringLiteral("c")));
}

void QSettingsProxyTests::fileNameIsReported()
{
  QTemporaryDir dir;
  QVERIFY(dir.isValid());
  const auto path = dir.filePath(QStringLiteral("proxy.conf"));

  QSettingsProxy proxy;
  proxy.load(path);
  QVERIFY(proxy.fileName().endsWith(QStringLiteral("proxy.conf")));
}

QTEST_MAIN(QSettingsProxyTests)
