/*
 * Synergy -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 TuPig Synergy Contributors
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "base/Log.h"

#include <QTest>

class ServerProxyTests : public QObject
{
  Q_OBJECT
private Q_SLOTS:
  void initTestCase();

  void unknownMessageReturnsUnknown();
  void serverNoopReturnsOkay();
  void serverCloseDisconnects();
  void keepAliveIsEchoed();
  void mouseMoveForwardsToClient();
  void enterAndLeaveForwardToClient();

private:
  Log m_log;
};
