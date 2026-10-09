/*
 * Synergy -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2021 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2008 Volker Lanz <https://github.com/Tupig/Tupig_synergy/issues>
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

class MSWindowsPowerManager
{
public:
  ~MSWindowsPowerManager();

  /**
   * @brief Prevents the system from sleep automatically
   */
  void disableSleep();

  /**
   * @brief Enable automatically sleeping
   */
  void enableSleep();
};
