/*
 * Synergy -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 TuPig
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "base/Log.h"

#include <QTest>

class CondVarTests : public QObject
{
  Q_OBJECT
private Q_SLOTS:
  // Test are run in order top to bottom
  void initTestCase();
  void waitReturnsTrueWhenSignalled();
  void waitTimesOutWithoutSignal();
  void broadcastWakesAllWaiters();

private:
  //! Installs the global logger; mt::Thread logs from the new thread and a
  //! null global log segfaults in release builds.
  Log m_log;
};
