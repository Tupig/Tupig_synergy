/*
 * Synergy -- mouse and keyboard sharing utility
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

#include <exception>

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

  // clean up jobs (pending removals are still referenced by m_jobs, or their
  // displaced jobs live in m_retiredJobs, so both loops below cover them)
  for (auto &entry : m_jobs) {
    delete entry.job;
  }
  for (auto *job : m_retiredJobs) {
    delete job;
  }
}

void SocketMultiplexer::addSocket(ISocket *socket, ISocketMultiplexerJob *job)
{
  assert(socket != nullptr);
  assert(job != nullptr);

  // break thread out of poll
  m_thread->unblockPollSocket();

  Lock lock(m_mutex.get());

  // a socket being (re)added must not be subject to an earlier removal
  for (auto it = m_pendingRemovals.begin(); it != m_pendingRemovals.end();) {
    if (*it == socket) {
      it = m_pendingRemovals.erase(it);
    } else {
      ++it;
    }
  }

  // check if socket already exists
  for (auto &entry : m_jobs) {
    if (entry.socket == socket) {
      // replace existing job.  Do NOT delete the displaced job here:
      // serviceThread may still be executing it from its snapshot.  Defer
      // the deletion to updateJobState().
      if (entry.job != job) {
        m_retiredJobs.push_back(entry.job);
        entry.job = job;
      }
      return;
    }
  }

  // add new socket/job pair
  m_jobs.push_back({socket, job});

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

  // mark for deferred removal — do NOT delete the job here as serviceThread
  // may still be executing it via its snapshot.  The job will be deleted in
  // updateJobState() after the service thread is done with it.
  for (auto &entry : m_jobs) {
    if (entry.socket == socket) {
      m_pendingRemovals.push_back(socket);
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
    // skip jobs marked for removal
    if (entry.job == nullptr) {
      continue;
    }
    bool pending = false;
    for (ISocket *s : m_pendingRemovals) {
      if (s == entry.socket) {
        pending = true;
        break;
      }
    }
    if (!pending) {
      snapshot.push_back(entry);
    }
  }

  return snapshot;
}

void SocketMultiplexer::updateJobState(const std::vector<JobTransition> &transitions)
{
  Lock lock(m_mutex.get());

  // apply job transitions incrementally.  Do NOT rebuild m_jobs from the
  // snapshot: sockets added or replaced concurrently by addSocket() must be
  // preserved (a full rebuild would drop them and leak their jobs).
  for (const auto &transition : transitions) {
    bool matched = false;
    for (auto it = m_jobs.begin(); it != m_jobs.end(); ++it) {
      if (it->socket != transition.socket || it->job != transition.previousJob) {
        continue;
      }
      matched = true;
      if (transition.nextJob != transition.previousJob) {
        // serviceThread is done with previousJob, so it is safe to delete
        delete transition.previousJob;
        if (transition.nextJob != nullptr) {
          it->job = transition.nextJob;
        } else {
          m_jobs.erase(it);
        }
      }
      break;
    }
    if (!matched && transition.nextJob != transition.previousJob && transition.nextJob != nullptr) {
      // the entry was replaced concurrently by addSocket(); the replacement
      // in m_jobs wins and the job created by run() is never installed, so
      // delete it to avoid a leak.  previousJob is owned by m_retiredJobs or
      // m_pendingRemovals and is handled below.
      delete transition.nextJob;
    }
  }

  // process pending removals: delete jobs and remove entries
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

  // delete jobs displaced by concurrent addSocket() calls
  for (auto *job : m_retiredJobs) {
    delete job;
  }
  m_retiredJobs.clear();

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

    // execute jobs and collect state transitions (no lock held)
    std::vector<JobTransition> transitions;
    transitions.reserve(snapshot.size());

    if (status != 0) {
      for (size_t i = 0; i < snapshot.size() && i < pfds.size(); ++i) {
        const auto &entry = snapshot[i];
        unsigned short revents = pfds[i].m_revents;
        bool read = ((revents & int(IArchNetwork::PollEventMask::In)) != 0);
        bool write = ((revents & int(IArchNetwork::PollEventMask::Out)) != 0);
        bool error =
            ((revents & (int(IArchNetwork::PollEventMask::Error) | int(IArchNetwork::PollEventMask::Invalid))) != 0);

        // run job
        ISocketMultiplexerJob *job = entry.job;
        ISocketMultiplexerJob *newJob = job;
        try {
          newJob = job->run(read, write, error);
        } catch (const std::exception &e) {
          // A job must never let an exception escape: this is the multiplexer's
          // service loop, and an uncaught exception unwinds through
          // Thread::threadFunc (which rethrows) into std::terminate, killing
          // the process. Treat it like Break: log, drop this socket's job, and
          // keep serving the other connections.
          LOG_ERR("exception in socket job, dropping connection: %s", e.what());
          newJob = nullptr;
        } catch (...) {
          LOG_ERR("unknown exception in socket job, dropping connection");
          newJob = nullptr;
        }

        if (newJob != job) {
          transitions.push_back({entry.socket, job, newJob});
        }
      }
    }

    // update state under lock
    updateJobState(transitions);
  }
}

//
// SocketMultiplexer singleton
//

SocketMultiplexer *SocketMultiplexer::getInstance()
{
  // The instance is intentionally never destroyed (it owns a service thread
  // and is used until process exit).  C++11 magic-static initialization is
  // thread-safe, unlike the previous check-then-set pattern which could
  // create two instances — and two service threads — on concurrent first use.
  static SocketMultiplexer *s_instance = new SocketMultiplexer;
  return s_instance;
}
