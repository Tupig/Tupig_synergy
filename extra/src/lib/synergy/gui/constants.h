/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2024 - 2026 Synergy App Ltd
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

const auto kUrlWebsite = QStringLiteral("https://github.com/Tupig/Tupig_synergy");

const auto kUrlGpl = QStringLiteral("https://www.gnu.org/licenses/old-licenses/gpl-2.0.html");
// EULA 指向项目 README（包含许可信息）
const auto kUrlEula = QString("%1#readme").arg(kUrlWebsite);

const auto kLink = R"(<a href="%1" style="color: %2">%3</a>)";

} // namespace synergy::gui
