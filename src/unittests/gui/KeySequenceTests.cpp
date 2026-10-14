/*
 * Synergy -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "KeySequenceTests.h"

#include "gui/KeySequence.h"

#include <QSettings>
#include <QTemporaryDir>

namespace {

QList<int> keysOf(const KeySequence &sequence)
{
  return sequence.sequence();
}

} // namespace

void KeySequenceTests::toString_controlShiftPlus_usesNamedPlus()
{
  KeySequence sequence;

  sequence.appendKey(Qt::Key_Control, Qt::ControlModifier);
  sequence.appendKey(Qt::Key_Shift, Qt::ControlModifier | Qt::ShiftModifier);
  QVERIFY(sequence.appendKey(Qt::Key_Plus, Qt::ControlModifier | Qt::ShiftModifier));

  QCOMPARE(sequence.toString(), QStringLiteral("Control+Shift+Plus"));
}

void KeySequenceTests::startsInvalid()
{
  KeySequence sequence;

  QVERIFY(!sequence.valid());
  QVERIFY(!sequence.isMouseButton());
  QVERIFY(sequence.toString().isEmpty());
  QVERIFY(keysOf(sequence).isEmpty());
  QCOMPARE(sequence.modifiers(), 0);
}

void KeySequenceTests::repeatedModifierIsNotDuplicated()
{
  KeySequence sequence;

  sequence.appendKey(Qt::Key_Control, Qt::ControlModifier);
  sequence.appendKey(Qt::Key_Control, Qt::ControlModifier);

  QCOMPARE(keysOf(sequence).size(), 1);
  QCOMPARE(sequence.modifiers(), static_cast<int>(Qt::ControlModifier));
}

void KeySequenceTests::mouseButtonsRenderAsDigits()
{
  KeySequence sequence;

  QVERIFY(sequence.appendMouseButton(Qt::LeftButton));
  QVERIFY(sequence.appendMouseButton(Qt::MiddleButton));

  QVERIFY(sequence.isMouseButton());
  QVERIFY(sequence.valid());
  QCOMPARE(sequence.toString(), QStringLiteral("1+3"));
}

void KeySequenceTests::rejectsUnknownKeysAndAltGr()
{
  KeySequence sequence;

  // AltGr cannot be part of a sequence
  QVERIFY(!sequence.appendKey(Qt::Key_AltGr, 0));

  // a key with no printable representation must be rejected and must not
  // validate the sequence
  QVERIFY(!sequence.appendKey(0x100000, 0));
  QVERIFY(!sequence.valid());
  QVERIFY(keysOf(sequence).isEmpty());
}

void KeySequenceTests::capsSequenceAtFourKeys()
{
  KeySequence sequence;

  QVERIFY(sequence.appendKey(Qt::Key_F1, 0));
  QVERIFY(sequence.appendKey(Qt::Key_F2, 0));
  QVERIFY(sequence.appendKey(Qt::Key_F3, 0));
  QVERIFY(sequence.appendKey(Qt::Key_F4, 0));
  sequence.appendKey(Qt::Key_F5, 0);

  QCOMPARE(keysOf(sequence).size(), 4);
  QCOMPARE(sequence.toString(), QStringLiteral("F1+F2+F3+F4"));
}

void KeySequenceTests::unicodeKeyUsesEscapeForm()
{
  KeySequence sequence;

  // U+4E2D (CJK) has no name and is rendered as a lowercase \u escape
  QVERIFY(sequence.appendKey(0x4E2D, 0));
  QCOMPARE(sequence.toString(), QStringLiteral("\\u4e2d"));
}

void KeySequenceTests::settingsRoundTripPreservesSequence()
{
  QTemporaryDir dir;
  QVERIFY(dir.isValid());

  KeySequence original;
  original.appendKey(Qt::Key_Control, Qt::ControlModifier);
  original.appendKey(Qt::Key_F5, Qt::ControlModifier);

  {
    QSettings settings(dir.filePath(QStringLiteral("sequence.conf")), QSettings::IniFormat);
    original.saveSettings(settings);
    settings.sync();
  }

  KeySequence loaded;
  {
    QSettings settings(dir.filePath(QStringLiteral("sequence.conf")), QSettings::IniFormat);
    loaded.loadSettings(settings);
  }

  QVERIFY(loaded.valid());
  QVERIFY(keysOf(loaded) == keysOf(original));
  QCOMPARE(loaded.toString(), original.toString());
}

QTEST_MAIN(KeySequenceTests)
