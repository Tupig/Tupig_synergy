/*
 * Synergy -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 TuPig Synergy Contributors
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "base/Log.h"

#include <QTest>

class IpcServerTests : public QObject
{
  Q_OBJECT
private Q_SLOTS:
  void initTestCase();
  void cleanupTestCase();

  void commandBeforeHelloRejected();
  void helloWithoutTokenRejected();
  void helloWithWrongTokenRejected();
  void helloWithWrongVersionReportsMismatch();
  void validHelloAcceptsCommands();
  void oversizedMessageDisconnects();

private:
  Log m_log;
  QString m_testDir;
  QString m_serverName;
};
