/*
 * Synergy -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <array>
#include <cstdint>
#include <string>

class XDGPowerManager
{
public:
  XDGPowerManager() = default;
  ~XDGPowerManager();

  /**
   * @brief Prevent the system from sleep
   */
  void disableSleep() const;

  /**
   * @brief Enables automatical sleep
   */
  void enableSleep() const;

  XDGPowerManager(const XDGPowerManager &) = delete;
  XDGPowerManager &operator=(const XDGPowerManager &) = delete;

  enum class InhibitScreenServices
  {
    kScreenSaver,
    kSessionManager
  };

private:
  bool inhibitScreenCall(InhibitScreenServices serviceID, bool state, std::string &error) const;
  bool sleepInhibitCall(bool state, InhibitScreenServices serviceID) const;

  //! Inhibit cookies returned by the screen saver / session manager, per
  //! service and per instance so two managers (for example a primary and a
  //! secondary screen) cannot clear each other's inhibition.
  mutable std::array<uint32_t, 2> m_cookies = {};
};
