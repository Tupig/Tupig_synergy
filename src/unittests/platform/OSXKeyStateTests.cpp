/*
 * Synergy -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Chris Rizzitello <https://github.com/Tupig/Tupig_synergy/issues>
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2011 Nick Bolton
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "OSXKeyStateTests.h"

#include "base/EventQueue.h"
#include "base/Stopwatch.h"

#define SHIFT_ID_L kKeyShift_L
#define SHIFT_ID_R kKeyShift_R
#define SHIFT_BUTTON 57
#define A_CHAR_ID 0x00000061
#define A_CHAR_BUTTON 001

namespace {

//! Release any synthetic keys on scope exit, including an early return from a
//! failed assertion, so the active session is never left with a stuck key.
class InjectedKeyGuard
{
public:
  explicit InjectedKeyGuard(OSXKeyState &keyState) : m_keyState(keyState)
  {
  }
  InjectedKeyGuard(const InjectedKeyGuard &) = delete;
  InjectedKeyGuard &operator=(const InjectedKeyGuard &) = delete;
  ~InjectedKeyGuard()
  {
    m_keyState.fakeAllKeysUp();
  }

private:
  OSXKeyState &m_keyState;
};

QString keyStateFailure(KeyButton button, bool expected, bool observed)
{
  return QStringLiteral("button %1: expected %2, last observed %3")
      .arg(button)
      .arg(expected ? QStringLiteral("pressed") : QStringLiteral("released"))
      .arg(observed ? QStringLiteral("pressed") : QStringLiteral("released"));
}

} // namespace

void OSXKeyStateTests::initTestCase()
{
  m_arch.init();
  m_log.setFilter(LogLevel::Level::Verbose);
}

void OSXKeyStateTests::mapModifiersFromOSX_OSXMask()
{
  synergy::KeyMap keyMap;
  EventQueue eventQueue;
  OSXKeyState keyState(&eventQueue, keyMap, {"en"}, true);

  KeyModifierMask outMask = 0;

  uint32_t shiftMask = 0 | kCGEventFlagMaskShift;
  outMask = keyState.mapModifiersFromOSX(shiftMask);
  QCOMPARE(outMask, KeyModifierShift);

  uint32_t ctrlMask = 0 | kCGEventFlagMaskControl;
  outMask = keyState.mapModifiersFromOSX(ctrlMask);
  QCOMPARE(outMask, KeyModifierControl);

  uint32_t altMask = 0 | kCGEventFlagMaskAlternate;
  outMask = keyState.mapModifiersFromOSX(altMask);
  QCOMPARE(outMask, KeyModifierAlt);

  uint32_t cmdMask = 0 | kCGEventFlagMaskCommand;
  outMask = keyState.mapModifiersFromOSX(cmdMask);
  QCOMPARE(outMask, KeyModifierSuper);

  uint32_t capsMask = 0 | kCGEventFlagMaskAlphaShift;
  outMask = keyState.mapModifiersFromOSX(capsMask);
  QCOMPARE(outMask, KeyModifierCapsLock);

  uint32_t numMask = 0 | kCGEventFlagMaskNumericPad;
  outMask = keyState.mapModifiersFromOSX(numMask);
  QCOMPARE(outMask, KeyModifierNumLock);
}

void OSXKeyStateTests::fakePollShift()
{
  synergy::KeyMap keyMap;
  EventQueue eventQueue;
  OSXKeyState keyState(&eventQueue, keyMap, {"en"}, true);
  keyState.updateKeyMap();
  InjectedKeyGuard guard(keyState);

  bool observed = false;

  keyState.fakeKeyDown(SHIFT_ID_L, 0, 1, "en");
  QVERIFY2(
      waitForKeyState(keyState, SHIFT_BUTTON, true, 5.0, observed),
      qPrintable(keyStateFailure(SHIFT_BUTTON, true, observed))
  );

  keyState.fakeKeyUp(1);
  QVERIFY2(
      waitForKeyState(keyState, SHIFT_BUTTON, false, 5.0, observed),
      qPrintable(keyStateFailure(SHIFT_BUTTON, false, observed))
  );

  keyState.fakeKeyDown(SHIFT_ID_R, 0, 2, "en");
  QVERIFY2(
      waitForKeyState(keyState, SHIFT_BUTTON, true, 5.0, observed),
      qPrintable(keyStateFailure(SHIFT_BUTTON, true, observed))
  );

  keyState.fakeKeyUp(2);
  QVERIFY2(
      waitForKeyState(keyState, SHIFT_BUTTON, false, 5.0, observed),
      qPrintable(keyStateFailure(SHIFT_BUTTON, false, observed))
  );
}

void OSXKeyStateTests::fakePollChar()
{
  synergy::KeyMap keyMap;
  EventQueue eventQueue;
  OSXKeyState keyState(&eventQueue, keyMap, {"en"}, true);
  keyState.updateKeyMap();
  InjectedKeyGuard guard(keyState);

  bool observed = false;

  keyState.fakeKeyDown(A_CHAR_ID, 0, 1, "en");
  QVERIFY2(
      waitForKeyState(keyState, A_CHAR_BUTTON, true, 5.0, observed),
      qPrintable(keyStateFailure(A_CHAR_BUTTON, true, observed))
  );

  keyState.fakeKeyUp(1);
  QVERIFY2(
      waitForKeyState(keyState, A_CHAR_BUTTON, false, 5.0, observed),
      qPrintable(keyStateFailure(A_CHAR_BUTTON, false, observed))
  );

  // HACK: delete the key in case it was typed into a text editor.
  // we should really set focus to an invisible window.
  keyState.fakeKeyDown(kKeyBackSpace, 0, 2, "en");
  keyState.fakeKeyUp(2);
}

void OSXKeyStateTests::fakePollCharWithModifier()
{
  synergy::KeyMap keyMap;
  EventQueue eventQueue;
  OSXKeyState keyState(&eventQueue, keyMap, {"en"}, true);
  keyState.updateKeyMap();
  InjectedKeyGuard guard(keyState);

  bool observed = false;

  keyState.fakeKeyDown(A_CHAR_ID, KeyModifierShift, 1, "en");
  QVERIFY2(
      waitForKeyState(keyState, A_CHAR_BUTTON, true, 5.0, observed),
      qPrintable(keyStateFailure(A_CHAR_BUTTON, true, observed))
  );

  keyState.fakeKeyUp(1);
  QVERIFY2(
      waitForKeyState(keyState, A_CHAR_BUTTON, false, 5.0, observed),
      qPrintable(keyStateFailure(A_CHAR_BUTTON, false, observed))
  );

  // HACK: delete the key in case it was typed into a text editor.
  // we should really set focus to an invisible window.
  keyState.fakeKeyDown(kKeyBackSpace, 0, 2, "en");
  keyState.fakeKeyUp(2);
}

bool OSXKeyStateTests::waitForKeyState(
    const OSXKeyState &keyState, KeyButton button, bool expected, double timeoutSeconds, bool &lastObserved
)
{
  constexpr double kPollInterval = 0.05;

  const auto isPressed = [&keyState](KeyButton button) {
    IKeyState::KeyButtonSet pressed;
    keyState.pollPressedKeys(pressed);
    return pressed.count(button) != 0;
  };

  Stopwatch timer;
  bool observed = isPressed(button);
  while (observed != expected && timer.getTime() < timeoutSeconds) {
    Arch::sleep(kPollInterval);
    observed = isPressed(button);
  }

  lastObserved = observed;
  return observed == expected;
}

QTEST_MAIN(OSXKeyStateTests)
