/*
 * Synergy -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Synergy Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "base/Log.h"

#include <QTest>

class MSWindowsClipboardConversionTests : public QObject
{
  Q_OBJECT
private Q_SLOTS:
  // Test are run in order top to bottom
  void initTestCase();
  void addText_invokesInjectedFacadeWithUtf16Data();
  void addText_convertsNonAsciiCharacters();
  void addHtml_writesRegisteredHtmlFormat();
  void addEmptyText_doesNotInvokeFacade();
  void setFacade_twice_replacesWithoutDeletingPrevious();

private:
  Log m_log;
};
