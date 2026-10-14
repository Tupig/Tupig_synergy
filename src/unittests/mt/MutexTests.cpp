/*
 * Synergy -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 TuPig
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "MutexTests.h"

#include "arch/Arch.h"
#include "base/IJob.h"
#include "mt/Lock.h"
#include "mt/Mutex.h"
#include "mt/Thread.h"

#include <atomic>

namespace {

//! Acquire the mutex, record it, and release it again.
class LockProbeJob : public IJob
{
public:
  LockProbeJob(Mutex &mutex, std::atomic<bool> &acquired) : m_mutex(mutex), m_acquired(acquired)
  {
  }

  void run() override
  {
    m_mutex.lock();
    m_acquired.store(true);
    m_mutex.unlock();
  }

private:
  Mutex &m_mutex;
  std::atomic<bool> &m_acquired;
};

} // namespace

void MutexTests::initTestCase()
{
  // The mutex primitives reach for the ARCH singleton, which only the
  // application normally installs.
  static Arch arch;
#if defined(Q_OS_WIN)
  arch.init();
#endif
}

void MutexTests::lockUnlockRoundTrip()
{
  Mutex mutex;
  mutex.lock();
  mutex.unlock();

  // a second lock/unlock cycle must succeed as well
  mutex.lock();
  mutex.unlock();
}

void MutexTests::scopedLockReleasesOnDestruction()
{
  Mutex mutex;
  {
    Lock lock(&mutex);
  }

  // if the scoped lock leaked the mutex, this would deadlock the test
  mutex.lock();
  mutex.unlock();
}

void MutexTests::copyConstructedMutexIsIndependent()
{
  Mutex first;
  Mutex second(first);

  // locking both succeeds only when the copy is an independent lock
  first.lock();
  second.lock();
  second.unlock();
  first.unlock();
}

void MutexTests::lockBlocksUntilOwnerUnlocks()
{
  Mutex mutex;
  std::atomic<bool> acquired{false};

  mutex.lock();

  // the helper blocks on the same mutex and only records the lock after the
  // unlock below
  Thread helper(new LockProbeJob(mutex, acquired));
  Arch::sleep(0.1);
  QVERIFY(!acquired.load());

  mutex.unlock();
  QVERIFY(helper.wait(5.0));
  QVERIFY(acquired.load());
}

QTEST_MAIN(MutexTests)
