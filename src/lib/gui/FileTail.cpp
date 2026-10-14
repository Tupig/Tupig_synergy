/*
 * Synergy -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-FileCopyrightText: (C) 2025 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "FileTail.h"

#include <QDebug>
#include <QFile>
#include <QFileInfo>
#include <QFileSystemWatcher>
#include <QObject>
#include <QTextStream>

namespace synergy::gui {

FileTail::FileTail(const QString &filePath, QObject *parent) : QObject(parent), m_watcher(new QFileSystemWatcher(this))
{
  setWatchedFile(filePath);
  connect(m_watcher, &QFileSystemWatcher::fileChanged, this, &FileTail::handleFileChanged);
}

void FileTail::setWatchedFile(const QString &filePath)
{
  if (filePath.isEmpty())
    return;

  if (m_file.fileName() == filePath)
    return;

  if (!m_file.fileName().isEmpty()) {
    m_watcher->removePath(m_file.fileName());
    m_file.close();
    m_file.setFileName(filePath);
  }

  m_file.setFileName(filePath);
  if (!m_file.open(QIODevice::ReadOnly | QIODevice::Text)) {
    qCritical() << "failed to open file for tail:" << filePath;
    return;
  }

  qDebug() << "starting file tail:" << filePath;
  m_watcher->addPath(filePath);
  m_lastPos = m_file.size();
}

void FileTail::handleFileChanged(const QString &path)
{
  if (path != m_file.fileName()) {
    return;
  }

  // the watcher drops the path when the file is renamed or removed (log
  // rotation); re-arm it so a recreated file is watched again
  if (!m_watcher->files().contains(path)) {
    m_watcher->addPath(path);
  }

  // rotation leaves this handle on the renamed file and truncation resets a
  // same-name file below our position; reopen and start from the beginning in
  // both cases so the recreated file is read
  const bool replaced = !QFileInfo::exists(path) || QFileInfo(path).size() < m_file.size();
  if (replaced || m_file.size() < m_lastPos) {
    m_file.close();
    if (!m_file.open(QIODevice::ReadOnly | QIODevice::Text)) {
      qWarning() << "failed to reopen tailed file:" << path;
      return;
    }
    m_lastPos = 0;
  }

  if (!m_file.seek(m_lastPos)) {
    qWarning() << "failed to seek in tailed file:" << path;
    return;
  }
  QTextStream stream(&m_file);
  while (!stream.atEnd()) {
    QString line = stream.readLine();
    Q_EMIT newLine(line);
  }
  m_lastPos = m_file.pos();
}

} // namespace synergy::gui
