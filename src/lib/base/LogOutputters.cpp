/*
 * Synergy -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 - 2026 Deskflow Developers
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "base/LogOutputters.h"
#include "arch/Arch.h"

#include <iostream>

#include <QFile>
#include <QString>

constexpr auto s_logFileSizeLimit = 1024 * 1024; //!< Max Log size before rotating (1Mb)

//! How many bytes to write before checking the log size again.  Checking on
//! every line is a stat-like syscall per line; the exact rotation point may
//! overshoot by up to this amount, which is irrelevant for a log.
constexpr auto s_logFileSizeCheckBytes = 64 * 1024;

//
// StopLogOutputter
//

void StopLogOutputter::open(const QString &)
{
  // do nothing
}

void StopLogOutputter::close()
{
  // do nothing
}

bool StopLogOutputter::write(LogLevel::Level, const QString &)
{
  return false;
}

//
// ConsoleLogOutputter
//

void ConsoleLogOutputter::open(const QString &)
{
  // do nothing
}

void ConsoleLogOutputter::close()
{
  // do nothing
}

bool ConsoleLogOutputter::write(LogLevel::Level level, const QString &msg)
{
  using enum LogLevel::Level;
  // std::endl already flushes the target stream
  if ((level >= Fatal) && (level <= Warning))
    std::cerr << qPrintable(msg) << std::endl;
  else
    std::cout << qPrintable(msg) << std::endl;
  return true;
}

void ConsoleLogOutputter::flush() const
{
  // do nothing
}

//
// SystemLogOutputter
//

void SystemLogOutputter::open(const QString &title)
{
  ARCH->openLog(title);
}

void SystemLogOutputter::close()
{
  ARCH->closeLog();
}

bool SystemLogOutputter::write(LogLevel::Level level, const QString &msg)
{
  ARCH->writeLog(level, msg);
  return true;
}

//
// SystemLogger
//

SystemLogger::SystemLogger(const QString &title, bool blockConsole)
{
  // redirect log messages
  if (blockConsole) {
    m_stop = new StopLogOutputter; // NOSONAR - Adopted by `Log`
    CLOG->insert(m_stop);
  }
  m_syslog = new SystemLogOutputter; // NOSONAR - Adopted by `Log`
  m_syslog->open(title);
  CLOG->insert(m_syslog);
}

SystemLogger::~SystemLogger()
{
  CLOG->remove(m_syslog);
  delete m_syslog;
  if (m_stop != nullptr) {
    CLOG->remove(m_stop);
    delete m_stop;
  }
}

//
// FileLogOutputter
//

FileLogOutputter::FileLogOutputter(const QString &logFile)
{
  setLogFilename(logFile);
}

void FileLogOutputter::setLogFilename(const QString &logFile)
{
  assert(logFile != nullptr);
  if (m_fileName == logFile) {
    return;
  }
  m_file.close();
  m_fileName = logFile;
  m_bytesSinceCheck = 0;
}

bool FileLogOutputter::ensureOpen()
{
  if (m_file.isOpen()) {
    return true;
  }
  return m_file.open(QFile::WriteOnly | QFile::Append);
}

void FileLogOutputter::rotateIfNeeded()
{
  m_bytesSinceCheck = 0;
  if (m_file.size() <= s_logFileSizeLimit) {
    return;
  }

  const auto oldFile = QStringLiteral("%1.1").arg(m_fileName);
  // rotate: drop any previous backup, then rename the current log into
  // its place; the next write recreates the log file from scratch
  m_file.close();
  QFile::remove(oldFile);
  QFile::rename(m_fileName, oldFile);
}

bool FileLogOutputter::write(LogLevel::Level, const QString &message)
{
  if (!ensureOpen()) {
    return false;
  }

  const auto line = message.toUtf8() + '\n';
  if (m_file.write(line) != line.size()) {
    return false;
  }
  // flush per line so log tails (the GUI log dock) see new lines promptly
  m_file.flush();

  m_bytesSinceCheck += line.size();
  if (m_bytesSinceCheck >= s_logFileSizeCheckBytes) {
    rotateIfNeeded();
  }

  return true;
}

void FileLogOutputter::open(const QString &)
{
  // do nothing
}

void FileLogOutputter::close()
{
  m_file.close();
}
