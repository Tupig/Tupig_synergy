/*
 * Synergy -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2024 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "Diagnostic.h"

#include "common/Settings.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QProcess>

namespace synergy::gui::diagnostic {

void restart()
{
  QString program = QCoreApplication::applicationFilePath();
  QStringList arguments = QCoreApplication::arguments();

  // look for and remove --reset option if found
  if (int resetIndex = arguments.indexOf("--reset"); resetIndex != -1)
    arguments.removeAt(resetIndex);

  qInfo("launching new process: %s", qPrintable(program));
  QProcess::startDetached(program, arguments);

  qDebug("exiting current process");
  QCoreApplication::exit();
}

void clearSettings(bool enableRestart)
{
  qDebug("clearing settings");
  Settings::proxy().clear();

  // Reset the windowGeometry
  Settings::setValue(Settings::Gui::WindowGeometry);

  // save but do not emit saving signal which will prevent the current state of
  // the app config and server configs from being applied.
  Settings::save(false);

  // Capture this before the file disappears. On installed Windows,
  // settingsPath() is ProgramData, while the user profile is next to the
  // settings file. Deleting ProgramData would remove the service token and
  // TLS material for every user. After the portable file is gone,
  // isPortableMode() would also flip to false.
  const auto settingsFile = Settings::settingsFile();
  const auto profilePath = QFileInfo(settingsFile).absolutePath();
  const bool portable = Settings::isPortableMode();

  qInfo("removing profile dir: %s", qPrintable(profilePath));
  if (!QDir(profilePath).removeRecursively())
    qWarning("cannot remove profile dir: %s", qPrintable(profilePath));

#ifdef Q_OS_WIN
  if (portable) {
    if (QDir().mkpath(profilePath)) {
      QFile file(settingsFile);
      if (!file.open(QIODevice::WriteOnly))
        qWarning("cannot recreate portable settings file: %s", qPrintable(settingsFile));
      else {
        file.write(" ", 1);
        file.close();
      }
    } else {
      qWarning("cannot recreate portable settings directory: %s", qPrintable(profilePath));
    }
  }
#endif

  if (enableRestart) {
    qDebug("restarting");
    restart();
  } else {
    qDebug("skipping restart");
  }
}

} // namespace synergy::gui::diagnostic
