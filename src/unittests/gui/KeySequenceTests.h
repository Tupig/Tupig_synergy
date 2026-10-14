/*
 * Synergy -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include <QTest>

class KeySequenceTests : public QObject
{
  Q_OBJECT
private Q_SLOTS:
  void toString_controlShiftPlus_usesNamedPlus();
  void startsInvalid();
  void repeatedModifierIsNotDuplicated();
  void mouseButtonsRenderAsDigits();
  void rejectsUnknownKeysAndAltGr();
  void capsSequenceAtFourKeys();
  void unicodeKeyUsesEscapeForm();
  void settingsRoundTripPreservesSequence();
};
