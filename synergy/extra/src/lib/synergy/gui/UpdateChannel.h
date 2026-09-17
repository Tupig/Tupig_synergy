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

#include <QString>

namespace synergy::gui {

/**
 * @brief The update channel (stable/beta) the user receives version notifications for.
 *
 * Synergy-only: upstream Deskflow has no track/channel concept, so this lives in the
 * overlay and persists to Synergy.extra.conf rather than upstream's allow-listed
 * settings file. The choice is sent to the version server as a `track=` query param.
 */
class UpdateChannel
{
public:
  // Persisted/UI values; also the track names the version server expects.
  inline static const QString Stable = QStringLiteral("stable");
  inline static const QString Beta = QStringLiteral("beta");

  /**
   * @brief The user's chosen channel; defaults to Stable when unset.
   */
  static QString current();

  /**
   * @brief Records the user's channel choice.
   */
  static void setCurrent(const QString &channel);

  /**
   * @brief Appends `track=<channel>` to the version-check URL for non-stable
   * channels (the param the version server expects, matching the 1.21 line),
   * leaving stable requests byte-identical so the server needs no change.
   */
  static void applyToVersionCheckUrl(QString &url);
};

} // namespace synergy::gui
