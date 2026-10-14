/*
 * Synergy -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 TuPig
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "StopwatchTests.h"

#include "base/Stopwatch.h"

#include <chrono>
#include <thread>

namespace {

void sleepSeconds(double seconds)
{
  std::this_thread::sleep_for(std::chrono::duration<double>(seconds));
}

} // namespace

void StopwatchTests::elapsedTimeGrows()
{
  Stopwatch stopwatch;
  sleepSeconds(0.05);

  const auto elapsed = stopwatch.getTime();
  QVERIFY(elapsed >= 0.03);
  QVERIFY(elapsed < 5.0);
}

void StopwatchTests::stopFreezesElapsedTime()
{
  Stopwatch stopwatch;
  sleepSeconds(0.03);

  stopwatch.stop();
  const auto frozen = stopwatch.getTime();

  sleepSeconds(0.05);
  QCOMPARE(stopwatch.getTime(), frozen);
  QVERIFY(stopwatch.isStopped());
}

void StopwatchTests::startResumesElapsedTime()
{
  Stopwatch stopwatch;
  sleepSeconds(0.03);
  stopwatch.stop();
  const auto frozen = stopwatch.getTime();

  sleepSeconds(0.03);
  stopwatch.start();
  sleepSeconds(0.03);

  QVERIFY(!stopwatch.isStopped());
  QVERIFY(stopwatch.getTime() > frozen);
}

void StopwatchTests::resetReturnsElapsedTimeAndRestarts()
{
  Stopwatch stopwatch;
  sleepSeconds(0.05);

  const auto beforeReset = stopwatch.reset();
  QVERIFY(beforeReset >= 0.03);

  // after a reset the clock starts from zero again
  QVERIFY(stopwatch.getTime() < beforeReset);
}

QTEST_MAIN(StopwatchTests)
