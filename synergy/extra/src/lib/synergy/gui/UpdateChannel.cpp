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

#include "UpdateChannel.h"

#include "common/Constants.h"
#include "common/Settings.h"

#include <QSettings>

namespace synergy::gui {

namespace {

const auto kChannelKey = QStringLiteral("update/channel");

QString extraFile()
{
  return QStringLiteral("%1/%2.extra.conf").arg(Settings::UserDir, kAppName);
}

} // namespace

QString UpdateChannel::current()
{
  QSettings extra(extraFile(), QSettings::IniFormat);
  const auto channel = extra.value(kChannelKey, Stable).toString();
  return channel == Beta ? Beta : Stable;
}

void UpdateChannel::setCurrent(const QString &channel)
{
  QSettings extra(extraFile(), QSettings::IniFormat);
  extra.setValue(kChannelKey, channel == Beta ? Beta : Stable);
  extra.sync();
}

void UpdateChannel::applyToVersionCheckUrl(QString &url)
{
  const auto channel = current();
  if (channel == Stable) {
    return;
  }

  url += (url.contains(QLatin1Char('?')) ? QLatin1Char('&') : QLatin1Char('?')) + QStringLiteral("track=") + channel;
}

} // namespace synergy::gui
