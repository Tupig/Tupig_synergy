/*
 * Synergy -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 TuPig
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "ThreadTests.h"

#include "arch/Arch.h"
#include "base/IJob.h"
#include "mt/Thread.h"

#include <atomic>

namespace {

//! Record that the thread body ran, then return normally.
class FlagJob : public IJob
{
public:
  explicit FlagJob(std::atomic<bool> &ran) : m_ran(ran)
  {
  }

  void run() override
  {
    m_ran.store(true);
  }

private:
  std::atomic<bool> &m_ran;
};

//! Terminate the thread through Thread::exit() with a known result.
class ExitJob : public IJob
{
public:
  explicit ExitJob(void *result) : m_result(result)
  {
  }

  void run() override
  {
    Thread::exit(m_result);
  }

private:
  void *m_result;
};

//! Loop on cancellation points until cancelled.
class SleepLoopJob : public IJob
{
public:
  void run() override
  {
    for (;;) {
      // Arch::sleep() is a cancellation point
      Arch::sleep(0.01);
    }
  }
};

} // namespace

void ThreadTests::initTestCase()
{
  // The thread primitives reach for the ARCH singleton, which only the
  // application normally installs.
  static Arch arch;
#if defined(Q_OS_WIN)
  arch.init();
#endif
}

void ThreadTests::waitsForCompletion()
{
  std::atomic<bool> ran{false};
  Thread thread(new FlagJob(ran));

  QVERIFY(thread.wait(5.0));
  QVERIFY(ran.load());
}

void ThreadTests::exitValueIsReturnedByGetResult()
{
  int sentinel = 0;
  Thread thread(new ExitJob(&sentinel));

  QCOMPARE(thread.getResult(), static_cast<void *>(&sentinel));
}

void ThreadTests::cancelStopsAtCancellationPoint()
{
  Thread thread(new SleepLoopJob);

  // let the thread reach the sleep loop, then cancel it; it must terminate
  // through the cancellation point within the timeout
  Arch::sleep(0.05);
  thread.cancel();
  QVERIFY(thread.wait(5.0));
}

void ThreadTests::currentThreadHandleIsStable()
{
  const auto first = Thread::getCurrentThread();
  const auto second = Thread::getCurrentThread();
  QVERIFY(first == second);

  Thread other(new SleepLoopJob);
  QVERIFY(!(other == first));
  other.cancel();
  QVERIFY(other.wait(5.0));
}

QTEST_MAIN(ThreadTests)
