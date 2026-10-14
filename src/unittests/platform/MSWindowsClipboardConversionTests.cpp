/*
 * Synergy -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Synergy Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "MSWindowsClipboardConversionTests.h"

#include "IMSWindowsClipboardFacade.h"
#include "MSWindowsClipboard.h"

namespace {

//! Captures what \c MSWindowsClipboard::add() hands to the clipboard facade.
/*!
Owning and freeing the converted HGLOBAL mirrors the real facade, where
\c SetClipboardData() takes ownership of the handle.
*/
class MockClipboardFacade : public IMSWindowsClipboardFacade
{
public:
  void write(HANDLE win32Data, UINT win32Format) override
  {
    ++m_writeCount;
    m_lastFormat = win32Format;
    m_lastBytes.clear();

    if (win32Data != nullptr) {
      const void *locked = GlobalLock(win32Data);
      if (locked != nullptr) {
        m_lastBytes = QByteArray(static_cast<const char *>(locked), static_cast<int>(GlobalSize(win32Data)));
        GlobalUnlock(win32Data);
      }
      GlobalFree(win32Data);
    }
  }

  int writeCount() const
  {
    return m_writeCount;
  }
  UINT lastFormat() const
  {
    return m_lastFormat;
  }
  const QByteArray &lastBytes() const
  {
    return m_lastBytes;
  }

private:
  int m_writeCount = 0;
  UINT m_lastFormat = 0;
  QByteArray m_lastBytes;
};

//! Text data produced by \c MSWindowsClipboardUTF16Converter is NUL-terminated UTF-16.
QString utf16TextOf(const QByteArray &bytes)
{
  return QString::fromUtf16(reinterpret_cast<const char16_t *>(bytes.constData()), (bytes.size() / 2) - 1);
}

} // namespace

void MSWindowsClipboardConversionTests::initTestCase()
{
  m_log.setFilter(LogLevel::Level::Verbose);
}

void MSWindowsClipboardConversionTests::addText_invokesInjectedFacadeWithUtf16Data()
{
  MockClipboardFacade facade;
  MSWindowsClipboard clipboard(nullptr, facade);

  clipboard.add(IClipboard::Format::Text, "plain text");

  QCOMPARE(facade.writeCount(), 1);
  QCOMPARE(facade.lastFormat(), static_cast<UINT>(CF_UNICODETEXT));
  QCOMPARE(utf16TextOf(facade.lastBytes()), QStringLiteral("plain text"));
}

void MSWindowsClipboardConversionTests::addText_convertsNonAsciiCharacters()
{
  MockClipboardFacade facade;
  MSWindowsClipboard clipboard(nullptr, facade);

  // UTF-8 bytes for U+4E2D U+6587, kept as escapes so the source stays ASCII.
  clipboard.add(IClipboard::Format::Text, "\xE4\xB8\xAD\xE6\x96\x87");

  QCOMPARE(facade.writeCount(), 1);
  QCOMPARE(utf16TextOf(facade.lastBytes()), QString::fromUtf8("\xE4\xB8\xAD\xE6\x96\x87"));
}

void MSWindowsClipboardConversionTests::addHtml_writesRegisteredHtmlFormat()
{
  MockClipboardFacade facade;
  MSWindowsClipboard clipboard(nullptr, facade);

  clipboard.add(IClipboard::Format::HTML, "<b>bold</b>");

  QCOMPARE(facade.writeCount(), 1);
  QCOMPARE(facade.lastFormat(), static_cast<UINT>(RegisterClipboardFormat(L"HTML Format")));
  QVERIFY(facade.lastBytes().contains("<b>bold</b>"));
}

void MSWindowsClipboardConversionTests::addEmptyText_doesNotInvokeFacade()
{
  MockClipboardFacade facade;
  MSWindowsClipboard clipboard(nullptr, facade);

  clipboard.add(IClipboard::Format::Text, "");

  QCOMPARE(facade.writeCount(), 0);
}

void MSWindowsClipboardConversionTests::setFacade_twice_replacesWithoutDeletingPrevious()
{
  MockClipboardFacade first;
  MockClipboardFacade second;

  MSWindowsClipboard clipboard(nullptr);
  clipboard.setFacade(first);
  clipboard.add(IClipboard::Format::Text, "one");
  clipboard.setFacade(second);
  clipboard.add(IClipboard::Format::Text, "two");

  QCOMPARE(first.writeCount(), 1);
  QCOMPARE(utf16TextOf(first.lastBytes()), QStringLiteral("one"));
  QCOMPARE(second.writeCount(), 1);
  QCOMPARE(utf16TextOf(second.lastBytes()), QStringLiteral("two"));
}

QTEST_MAIN(MSWindowsClipboardConversionTests)
