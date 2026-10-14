/*
 * Synergy -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Synergy App Ltd
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
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

#include <QSettings>
#include <QString>
#include <QVariant>

#include <optional>
#include <utility>

namespace synergy::gui::migration {

/**
 * @brief Map one legacy Synergy 1.x setting onto the new settings layout.
 *
 * Pure key/value transformation with no I/O, so it is unit-testable without
 * touching the real settings files.
 *
 * @return the new key/value pair, or std::nullopt for obsolete keys.
 */
std::optional<std::pair<QString, QVariant>> mapLegacyKey(const QString &oldKey, const QVariant &value);

/**
 * @brief Decide whether a QSettings store contains legacy-format settings.
 *
 * File-existence is not a usable signal: on Linux the legacy NativeFormat
 * path coincides with the new IniFormat path, so sentinel keys discriminate.
 */
bool looksLikeLegacySettings(const QSettings &legacy);

} // namespace synergy::gui::migration
