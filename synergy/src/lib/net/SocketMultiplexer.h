/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2004 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <memory>
#include <vector>

template <class T> class CondVar;
class Mutex;
class Thread;
class ISocket;
class ISocketMultiplexerJob;

//! Socket multiplexer
/*!
A socket multiplexer services multiple sockets simultaneously.
*/
class SocketMultiplexer
{
public:
  SocketMultiplexer();
  SocketMultiplexer(SocketMultiplexer const &) = delete;
  SocketMultiplexer(SocketMultiplexer &&) = delete;
  ~SocketMultiplexer();

  SocketMultiplexer &operator=(SocketMultiplexer const &) = delete;
  SocketMultiplexer &operator=(SocketMultiplexer &&) = delete;

  //! @name manipulators
  //@{

  void addSocket(ISocket *, ISocketMultiplexerJob *);

  void removeSocket(ISocket *);

  //@}
  //! @name accessors
  //@{

  // maybe belongs on ISocketMultiplexer
  static SocketMultiplexer *getInstance();

  //@}

private:
  // Service job entry: pairs a socket key with its current job.
  struct SocketJobEntry {
    ISocket *socket = nullptr;
    ISocketMultiplexerJob *job = nullptr;
  };

  // Snapshot of job list for lock-free iteration in serviceThread.
  using JobSnapshot = std::vector<SocketJobEntry>;

  // Service sockets in a loop.
  [[noreturn]] void serviceThread(const void *);

  // Build a snapshot of active jobs under lock, then unlock.
  JobSnapshot buildJobSnapshot();

  // Update internal state after jobs have been executed.
  void updateJobState(const JobSnapshot &snapshot);

private:
  std::unique_ptr<Mutex> m_mutex;
  std::unique_ptr<Thread> m_thread;

  // Condition variable: signaled when m_jobsReady changes or on shutdown.
  std::unique_ptr<CondVar<bool>> m_jobsReady;

  // Flat list of active jobs (no cursor markers).  m_mutex protects all access.
  std::vector<SocketJobEntry> m_jobs;

  // Pending removals: sockets to erase from m_jobs after execution.
  std::vector<ISocket *> m_pendingRemovals;

  // Flag indicating m_jobs has changed since last snapshot.
  bool m_update = false;
};
