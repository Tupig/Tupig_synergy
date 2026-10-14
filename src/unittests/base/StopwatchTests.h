/*
 * Synergy -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 TuPig
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include <QTest>

class StopwatchTests : public QObject
{
  Q_OBJECT
private Q_SLOTS:
  // Test are run in order top to bottom
  void elapsedTimeGrows();
  void stopFreezesElapsedTime();
  void startResumesElapsedTime();
  void resetReturnsElapsedTimeAndRestarts();
};
