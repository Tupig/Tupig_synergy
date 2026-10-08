/*
 * Synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2026 Synergy App Ltd
 *
 * This package is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * found in the file LICENSE that should have accompanied this file.
 *
 * This package is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

class QDialog;

namespace synergy::gui {

class SettingsScope
{
public:
  /**
   * @brief Whether the user has chosen the system-scope settings file.
   * Persisted in Synergy.extra.conf so it survives restarts.
   */
  static bool preferSystem();

  /**
   * @brief Records the user's scope preference.
   */
  static void setPreferSystem(bool prefer);

  /**
   * @brief Whether the current process can write to the system-scope settings
   * directory. False on Linux without root, often false on Windows without admin.
   */
  static bool isSystemWritable();

  /**
   * @brief Apply a scope switch: copy settings to the destination scope,
   * persist the preference, prompt the user to restart.
   *
   * @return false if the switch was refused (e.g., system scope not writable
   * on this user). Caller should revert any UI state on false.
   */
  static bool switchTo(QDialog *parent, bool toSystem);
};

} // namespace synergy::gui
