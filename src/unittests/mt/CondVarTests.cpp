/*
 * Synergy -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 TuPig
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "CondVarTests.h"

#include "arch/Arch.h"
#include "base/IJob.h"
#include "mt/CondVar.h"
#include "mt/Mutex.h"
#include "mt/Thread.h"

#include <atomic>

namespace {

//! Signal the condition variable after a short delay.
class NotifyJob : public IJob
{
public:
  NotifyJob(Mutex &mutex, CondVar<bool> &ready) : m_mutex(mutex), m_ready(ready)
  {
  }

  void run() override
  {
    Arch::sleep(0.1);
    m_mutex.lock();
    m_ready = true;
    m_ready.signal();
    m_mutex.unlock();
  }

private:
  Mutex &m_mutex;
  CondVar<bool> &m_ready;
};

//! Wait until the condition variable's value flips, then count the wake-up.
class WaiterJob : public IJob
{
public:
  WaiterJob(Mutex &mutex, CondVar<bool> &ready, std::atomic<int> &woken)
      : m_mutex(mutex),
        m_ready(ready),
        m_woken(woken)
  {
  }

  void run() override
  {
    m_mutex.lock();
    while (!static_cast<bool>(m_ready)) {
      m_ready.wait(5.0);
    }
    m_woken.fetch_add(1);
    m_mutex.unlock();
  }

private:
  Mutex &m_mutex;
  CondVar<bool> &m_ready;
  std::atomic<int> &m_woken;
};

} // namespace

void CondVarTests::initTestCase()
{
  // The condition variable primitives reach for the ARCH singleton, which
  // only the application normally installs.
  static Arch arch;
#if defined(Q_OS_WIN)
  arch.init();
#endif
}

void CondVarTests::waitReturnsTrueWhenSignalled()
{
  Mutex mutex;
  CondVar<bool> ready(&mutex, false);

  Thread notifier(new NotifyJob(mutex, ready));

  mutex.lock();
  QVERIFY(ready.wait(5.0));
  QVERIFY(static_cast<bool>(ready));
  mutex.unlock();

  QVERIFY(notifier.wait(5.0));
}

void CondVarTests::waitTimesOutWithoutSignal()
{
  Mutex mutex;
  CondVar<bool> ready(&mutex, false);

  mutex.lock();
  QVERIFY(!ready.wait(0.05));
  QVERIFY(!static_cast<bool>(ready));
  mutex.unlock();
}

void CondVarTests::broadcastWakesAllWaiters()
{
  Mutex mutex;
  CondVar<bool> ready(&mutex, false);
  std::atomic<int> woken{0};

  Thread first(new WaiterJob(mutex, ready, woken));
  Thread second(new WaiterJob(mutex, ready, woken));

  // give both threads time to enter the wait
  Arch::sleep(0.1);

  mutex.lock();
  ready = true;
  ready.broadcast();
  mutex.unlock();

  QVERIFY(first.wait(5.0));
  QVERIFY(second.wait(5.0));
  QCOMPARE(woken.load(), 2);
}

QTEST_MAIN(CondVarTests)
