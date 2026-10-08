/*
 * Synergy -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 TuPig
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "base/Log.h"

#include <QTest>

class NetworkTransportFactoryTests : public QObject
{
  Q_OBJECT
private Q_SLOTS:
  void init();
  void cleanup();
  void defaultTypeIsLegacy();
  void envForcesLegacy();
  void envForcesQt();
  void qtCreationPaths();
  void legacyWithoutFactoryReturnsNull();
  void qtIsAvailable();

private:
  // Installs the global CLOG instance; without it the factory's LOG_* calls
  // dereference a null logger and crash.
  Log m_log;
};
