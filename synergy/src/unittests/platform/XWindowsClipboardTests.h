/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Chris Rizzitello <sithlord48@gmail.com>
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "base/Log.h"

// Qt headers that pull in qtextstream.h must precede X11 headers (XWindowsClipboard.h
// → Xlib.h) because X11 '#define Status int' breaks qtextstream.h's 'enum Status'.
#include <QTest>

#if !WINAPI_LIBEI && !WINAPI_PORTAL
#include "platform/XWindowsClipboard.h"
#endif

class XWindowsClipboardTests : public QObject
{
  Q_OBJECT
private Q_SLOTS:
  // Test are run in order top to bottom
  void defaultCtor();
  // Tests only work on X Windows
#if !WINAPI_LIBEI && !WINAPI_PORTAL
  void initTestCase();
  void cleanupTestCase();
  void open();
  void singleFormat();
#endif
private:
  Log m_log;
#if !WINAPI_LIBEI && !WINAPI_PORTAL
  const std::string m_testString = "deskflow test string";
  const std::string m_testString2 = "Another String";
  Display *m_display;
  Window m_window;
  XWindowsClipboard &getClipboard();
  std::unique_ptr<XWindowsClipboard> m_clipboard;
#endif
};
