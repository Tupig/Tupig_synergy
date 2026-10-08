/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 TuPig
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "base/Log.h"

#include <QTemporaryDir>
#include <QTest>

class FileTransferOutboundTests : public QObject
{
  Q_OBJECT
private Q_SLOTS:
  void initTestCase();
  void init();
  void cleanup();

  void disabledSendsNothing();
  void emptyPathsSendNothing();
  void sendsReadableFiles();
  void trimsToMaxFileCount();
  void dropsFilesAboveMaxSize();

private:
  QString writeFile(const QString &name, const QByteArray &contents);

  QTemporaryDir m_dir;
  Log m_log;
};
