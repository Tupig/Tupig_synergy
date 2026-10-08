/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 TuPig
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "base/Log.h"

#include <QTest>

class InputValidatorTests : public QObject
{
  Q_OBJECT
private Q_SLOTS:
  // Test are run in order top to bottom
  void initTestCase();
  void modifierMaskAcceptsDefinedBits();
  void modifierMaskAcceptsCapsLock();
  void modifierMaskRejectsUndefinedBits();
  void sanitizeModifierMaskClearsUndefinedBits();
  void sanitizeModifierMaskKeepsDefinedBits();
  void knownButtonIdAcceptsLegalValues();
  void knownButtonIdAcceptsX11ScrollWheel();
  void knownButtonIdRejectsUndefinedValue();
  void rateLimitAllowsEventsUnderLimit();
  void rateLimitBlocksOnceWindowIsFull();
  void rateLimitWindowSlides();
  void rateLimitIsPerKey();
  void setMaxEventsPerSecondRejectsZero();
  void blockedCombinationsDefaultToEmpty();
  void blockedCombinationMatchesExactKeyAndModifiers();
  void blockedCombinationToleratesLockModifiers();
  void blockedCombinationDoesNotMatchWrongModifiers();
  void blockedCombinationIgnoresUnrelatedKeys();
  void blockedCombinationWithZeroMaskBlocksKeyAlone();
  void parseBlockedCombinationAcceptsHexAndDecimal();
  void parseBlockedCombinationAcceptsMissingMask();
  void parseBlockedCombinationSkipsMalformedEntries();
  void parseBlockedCombinationRejectsUndefinedModifierBits();
  void parseBlockedCombinationKeepsValidEntriesAroundBadOnes();

private:
  Log m_log;
};
