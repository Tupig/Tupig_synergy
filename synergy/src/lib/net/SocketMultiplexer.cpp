/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2004 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "net/SocketMultiplexer.h"

#include "arch/Arch.h"
#include "arch/ArchException.h"
#include "base/Log.h"
#include "base/TMethodJob.h"
#include "mt/CondVar.h"
#include "mt/Lock.h"
#include "mt/Mutex.h"
#include "mt/Thread.h"
#include "net/ISocketMultiplexerJob.h"

//
// SocketMultiplexer
//

SocketMultiplexer::SocketMultiplexer()
    : m_mutex(std::make_unique<Mutex>()),
      m_jobsReady(std::make_unique<CondVar<bool>>(m_mutex.get(), false))
{
  // start thread
  auto tMethodJob = new TMethodJob<SocketMultiplexer>(this, &SocketMultiplexer::serviceThread);
  m_thread = std::make_unique<Thread>(tMethodJob);
}

SocketMultiplexer::~SocketMultiplexer()
{
  m_thread->cancel();
  m_thread->unblockPollSocket();
  m_thread->wait();
  // m_thread, m_jobsReady, m_mutex are all std::unique_ptr — automatically cleaned up.

  // clean up jobs
  for (auto &entry : m_jobs) {
    delete entry.job;
  }
}

void SocketMultiplexer::addSocket(ISocket *socket, ISocketMultiplexerJob *job)
{
  assert(socket != nullptr);
  assert(job != nullptr);

  // break thread out of poll
  m_thread->unblockPollSocket();

  Lock lock(m_mutex.get());

  // check if socket already exists
  for (auto &entry : m_jobs) {
    if (entry.socket == socket) {
      // replace existing job
      if (entry.job != job) {
        delete entry.job;
        entry.job = job;
      }
      m_update = true;
      return;
    }
  }

  // add new socket/job pair
  m_jobs.push_back({socket, job});
  m_update = true;

  // signal that jobs are ready
  if (!*m_jobsReady) {
    *m_jobsReady = true;
    m_jobsReady->signal();
  }
}

void SocketMultiplexer::removeSocket(ISocket *socket)
{
  assert(socket != nullptr);

  // break thread out of poll
  m_thread->unblockPollSocket();

  Lock lock(m_mutex.get());

  // mark for removal
  for (auto &entry : m_jobs) {
    if (entry.socket == socket && entry.job != nullptr) {
      delete entry.job;
      entry.job = nullptr;
      m_update = true;
      return;
    }
  }
}

SocketMultiplexer::JobSnapshot SocketMultiplexer::buildJobSnapshot()
{
  Lock lock(m_mutex.get());

  JobSnapshot snapshot;
  snapshot.reserve(m_jobs.size());

  for (const auto &entry : m_jobs) {
    if (entry.job != nullptr) {
      snapshot.push_back(entry);
    }
  }

  m_update = false;
  return snapshot;
}

void SocketMultiplexer::updateJobState(const JobSnapshot &snapshot)
{
  Lock lock(m_mutex.get());

  // rebuild m_jobs from snapshot (jobs may have changed during execution)
  m_jobs.clear();
  m_jobs.reserve(snapshot.size());

  for (const auto &entry : snapshot) {
    m_jobs.push_back(entry);
  }

  // process pending removals
  for (ISocket *socket : m_pendingRemovals) {
    for (auto it = m_jobs.begin(); it != m_jobs.end();) {
      if (it->socket == socket) {
        delete it->job;
        it = m_jobs.erase(it);
      } else {
        ++it;
      }
    }
  }
  m_pendingRemovals.clear();

  // update ready state
  bool isReady = !m_jobs.empty();
  if (*m_jobsReady != isReady) {
    *m_jobsReady = isReady;
    m_jobsReady->signal();
  }
}

[[noreturn]] void SocketMultiplexer::serviceThread(const void *)
{
  std::vector<IArchNetwork::PollEntry> pfds;

  // service the connections
  for (;;) {
    Thread::testCancel();

    // wait until there are jobs to handle
    {
      Lock lock(m_mutex.get());
      while (!(bool)*m_jobsReady) {
        m_jobsReady->wait();
      }
    }

    // build snapshot under lock, then release lock for execution
    JobSnapshot snapshot = buildJobSnapshot();

    // collect poll entries from snapshot
    pfds.clear();
    pfds.reserve(snapshot.size());

    for (const auto &entry : snapshot) {
      IArchNetwork::PollEntry pfd;
      pfd.m_socket = entry.job->getSocket();
      pfd.m_events = 0;
      if (entry.job->isReadable()) {
        pfd.m_events |= IArchNetwork::PollEventMask::In;
      }
      if (entry.job->isWritable()) {
        pfd.m_events |= IArchNetwork::PollEventMask::Out;
      }
      pfds.push_back(pfd);
    }

    int status;
    try {
      // check for status — use bounded timeout (seconds) so the service thread
      // can periodically wake up and process pending jobs even when
      // no socket activity occurs (e.g. keepalive timers).
      static const double kPollTimeout = 1.0;
      if (!pfds.empty()) {
        status = ARCH->pollSocket(&pfds[0], static_cast<int>(pfds.size()), kPollTimeout);
      } else {
        // no sockets to poll — sleep briefly to avoid busy-spinning
        ARCH->sleep(0.1);
        status = 0;
      }
    } catch (ArchNetworkException &e) {
      LOG_WARN("error in socket multiplexer: %s", e.what());
      status = 0;
    }

    // execute jobs and collect results (no lock held)
    if (status != 0) {
      JobSnapshot updatedSnapshot;
      updatedSnapshot.reserve(snapshot.size());

      for (size_t i = 0; i < snapshot.size() && i < pfds.size(); ++i) {
        const auto &entry = snapshot[i];
        unsigned short revents = pfds[i].m_revents;
        bool read = ((revents & int(IArchNetwork::PollEventMask::In)) != 0);
        bool write = ((revents & int(IArchNetwork::PollEventMask::Out)) != 0);
        bool error =
            ((revents & (int(IArchNetwork::PollEventMask::Error) | int(IArchNetwork::PollEventMask::Invalid))) != 0);

        // run job
        ISocketMultiplexerJob *job = entry.job;
        ISocketMultiplexerJob *newJob = job->run(read, write, error);

        if (newJob != job) {
          delete job;
          updatedSnapshot.push_back({entry.socket, newJob});
        } else {
          updatedSnapshot.push_back(entry);
        }
      }

      // update state under lock
      updateJobState(updatedSnapshot);
    } else {
      // no status change — keep snapshot as-is
      updateJobState(snapshot);
    }
  }
}

//
// SocketMultiplexer singleton
//

SocketMultiplexer *SocketMultiplexer::getInstance()
{
  static SocketMultiplexer *s_instance = nullptr;
  if (s_instance == nullptr) {
    s_instance = new SocketMultiplexer;
  }
  return s_instance;
}
