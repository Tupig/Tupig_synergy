/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2024 - 2026 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "common/Constants.h"

#include <QString>

// important: this is used for settings paths on some platforms,
// and must not be a url. qt automatically converts this to reverse domain
// notation (rdn), e.g. org.deskflow
const auto kOrgDomain = QString::fromUtf8(kAppDomain);

// Destination for "Get help" / "Report bug" in the menu and for the
// "report a bug" link shown in error dialogs. Both are handled by the project's
// GitHub issue tracker.
const auto kUrlHelp = QStringLiteral("https://github.com/Tupig/TuPig_Product/issues");

#if defined(Q_OS_LINUX)
const auto kUrlGnomeTrayFix = QStringLiteral("https://extensions.gnome.org/extension/615/appindicator-support/");
#endif
