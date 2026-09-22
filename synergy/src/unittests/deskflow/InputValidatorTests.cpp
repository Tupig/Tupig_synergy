/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 TuPig
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "InputValidatorTests.h"

#include "deskflow/input/InputValidator.h"
#include "deskflow/input/KeyTypes.h"
#include "deskflow/input/MouseTypes.h"

#include <chrono>

using namespace std::chrono_literals;

namespace {
constexpr auto kWindow = 1s;
}

void InputValidatorTests::initTestCase()
{
  m_log.setFilter(LogLevel::Level::Verbose);
}

void InputValidatorTests::modifierMaskAcceptsDefinedBits()
{
  // Every bit KeyTypes.h declares must be accepted, or normal typing breaks.
  QVERIFY(InputValidator::isValidModifierMask(0));
  QVERIFY(InputValidator::isValidModifierMask(KeyModifierShift));
  QVERIFY(InputValidator::isValidModifierMask(KeyModifierControl | KeyModifierAlt));
  QVERIFY(InputValidator::isValidModifierMask(
      KeyModifierShift | KeyModifierControl | KeyModifierAlt | KeyModifierMeta
  ));
  QVERIFY(InputValidator::isValidModifierMask(KeyModifierSuper));
  QVERIFY(InputValidator::isValidModifierMask(KeyModifierAltGr));
  QVERIFY(InputValidator::isValidModifierMask(KeyModifierLevel5Lock));
}

void InputValidatorTests::modifierMaskAcceptsCapsLock()
{
  // Regression guard: CapsLock sits at bit 12, outside the 0x1F window a
  // hand-written mask would use. Rejecting it would interrupt sharing whenever
  // the user had CapsLock on.
  QVERIFY(InputValidator::isValidModifierMask(KeyModifierCapsLock));
  QVERIFY(InputValidator::isValidModifierMask(KeyModifierCapsLock | KeyModifierShift));
}

void InputValidatorTests::modifierMaskRejectsUndefinedBits()
{
  // 0x8000 is not a modifier in KeyTypes.h, so it can only be malformed input.
  QVERIFY(!InputValidator::isValidModifierMask(0x8000));
  QVERIFY(!InputValidator::isValidModifierMask(KeyModifierShift | 0x8000));
}

void InputValidatorTests::sanitizeModifierMaskClearsUndefinedBits()
{
  QCOMPARE(InputValidator::sanitizeModifierMask(0xFFFF), static_cast<KeyModifierMask>(0x107F));
  QCOMPARE(
      InputValidator::sanitizeModifierMask(0x8000 | KeyModifierControl),
      static_cast<KeyModifierMask>(KeyModifierControl)
  );
}

void InputValidatorTests::sanitizeModifierMaskKeepsDefinedBits()
{
  const auto legitimate = static_cast<KeyModifierMask>(
      KeyModifierShift | KeyModifierControl | KeyModifierAlt | KeyModifierCapsLock
  );
  QCOMPARE(InputValidator::sanitizeModifierMask(legitimate), legitimate);
}

void InputValidatorTests::knownButtonIdAcceptsLegalValues()
{
  QVERIFY(InputValidator::isKnownButtonId(kButtonNone));
  QVERIFY(InputValidator::isKnownButtonId(kButtonLeft));
  QVERIFY(InputValidator::isKnownButtonId(kButtonMiddle));
  QVERIFY(InputValidator::isKnownButtonId(kButtonRight));
  QVERIFY(InputValidator::isKnownButtonId(kButtonExtra0));
  QVERIFY(InputValidator::isKnownButtonId(kButtonExtra1));
}

void InputValidatorTests::knownButtonIdAcceptsX11ScrollWheel()
{
  // Regression guard: the X11 scroll wheel uses ids 254 and 255, which a
  // "1..32" style range check would have rejected, breaking wheel scrolling.
  QVERIFY(InputValidator::isKnownButtonId(kX11ScrollWheelUp));
  QVERIFY(InputValidator::isKnownButtonId(kX11ScrollWheelDown));
  QVERIFY(InputValidator::isKnownButtonId(kX11ScrollWheelLeft));
  QVERIFY(InputValidator::isKnownButtonId(kX11ScrollWheelRight));
  QCOMPARE(static_cast<int>(kX11ScrollWheelUp), 255);
  QCOMPARE(static_cast<int>(kX11ScrollWheelDown), 254);
}

void InputValidatorTests::knownButtonIdRejectsUndefinedValue()
{
  // Not a documented id; callers log this but must not drop the event.
  QVERIFY(!InputValidator::isKnownButtonId(42));
}

void InputValidatorTests::rateLimitAllowsEventsUnderLimit()
{
  InputValidator validator;
  const auto now = std::chrono::steady_clock::now();

  for (uint32_t i = 0; i < validator.maxEventsPerSecond(); ++i) {
    QVERIFY(!validator.isRateLimited(kKeyDelete, now));
  }
}

void InputValidatorTests::rateLimitBlocksOnceWindowIsFull()
{
  InputValidator validator;
  const auto now = std::chrono::steady_clock::now();

  for (uint32_t i = 0; i < validator.maxEventsPerSecond(); ++i) {
    validator.isRateLimited(kKeyDelete, now);
  }

  // The window is now saturated, so the next event must be dropped.
  QVERIFY(validator.isRateLimited(kKeyDelete, now));
}

void InputValidatorTests::rateLimitWindowSlides()
{
  InputValidator validator;
  const auto start = std::chrono::steady_clock::now();

  for (uint32_t i = 0; i < validator.maxEventsPerSecond(); ++i) {
    validator.isRateLimited(kKeyDelete, start);
  }
  QVERIFY(validator.isRateLimited(kKeyDelete, start));

  // Past the window the old timestamps expire, so events flow again.
  QVERIFY(!validator.isRateLimited(kKeyDelete, start + kWindow + 1ms));
}

void InputValidatorTests::rateLimitIsPerKey()
{
  InputValidator validator;
  const auto now = std::chrono::steady_clock::now();

  for (uint32_t i = 0; i < validator.maxEventsPerSecond(); ++i) {
    validator.isRateLimited(kKeyDelete, now);
  }

  // Saturating one key must not throttle an unrelated one.
  QVERIFY(validator.isRateLimited(kKeyDelete, now));
  QVERIFY(!validator.isRateLimited(kKeyBackSpace, now));
}

void InputValidatorTests::setMaxEventsPerSecondRejectsZero()
{
  InputValidator validator;
  const auto initial = validator.maxEventsPerSecond();

  validator.setMaxEventsPerSecond(0);
  QCOMPARE(validator.maxEventsPerSecond(), initial);

  validator.setMaxEventsPerSecond(10);
  QCOMPARE(validator.maxEventsPerSecond(), static_cast<uint32_t>(10));
}

QTEST_MAIN(InputValidatorTests)
