/*
 * Synergy -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 TuPig
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <QTest>

class ScreenSetupModelTests : public QObject
{
  Q_OBJECT
private Q_SLOTS:
  void clampsRowsToScreenList();
  void clampsGridSize();
  void outOfRangeIndexesAreInvalid();
  void inRangeIndexesHaveNames();
};
