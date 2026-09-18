/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "InputValidator.h"

#include "base/Log.h"

#include <algorithm>

// Modifier key bitmasks (cross-platform, no platform headers needed)
// These match the Deskflow KeyModifierMask values from KeyTypes.h
static constexpr uint32_t kModifierShift = 0x01;
static constexpr uint32_t kModifierControl = 0x02;
static constexpr uint32_t kModifierAlt = 0x04;
static constexpr uint32_t kModifierSuper = 0x08; // Cmd on macOS, Win on Windows
static constexpr uint32_t kModifierAltGr = 0x10;
static constexpr uint32_t kModifierMask = 0x1F;

// Sensitive key combinations (modifier bitmask patterns)
// Ctrl+Alt+Delete: Windows security screen
static constexpr uint32_t kSensitiveCtrlAltDel = kModifierControl | kModifierAlt;
// Cmd+Q: Force quit on macOS (Cmd maps to Super)
static constexpr uint32_t kSensitiveCmdQ = kModifierSuper;
// Ctrl+Alt+Backspace: X server kill on Linux
static constexpr uint32_t kSensitiveCtrlAltBksp = kModifierControl | kModifierAlt;

// Key codes for sensitive keys (generic values, platform-agnostic)
static constexpr uint32_t kKeyDelete = 0xFFFF;
static constexpr uint32_t kKeyBackspace = 0x08;
static constexpr uint32_t kKeyQ = 0x51; // 'Q' in ASCII

bool InputValidator::isValidKeyCode(uint32_t keyCode)
{
  // Key code 0 is invalid; valid range is [1, 0xFFFF]
  // This covers X11 keysyms, Windows virtual keys, and macOS key codes
  return keyCode != 0 && keyCode <= 0xFFFF;
}

bool InputValidator::isValidButtonId(uint32_t buttonId)
{
  // Mouse buttons 1-32 (covers all standard and extended buttons)
  return buttonId >= 1 && buttonId <= 32;
}

bool InputValidator::isValidModifierMask(uint32_t mask)
{
  // Only bits 0-15 are valid for modifier masks
  return (mask & ~kModifierMask) == 0;
}

bool InputValidator::isSensitiveCombination(uint32_t keyCode, uint32_t modifierMask)
{
  // Ctrl+Alt+Delete — Windows security screen / task manager
  if ((modifierMask & kSensitiveCtrlAltDel) == kSensitiveCtrlAltDel &&
      keyCode == kKeyDelete) {
    LOG_WARN("blocked sensitive combination: Ctrl+Alt+Delete");
    return true;
  }

  // Cmd+Q — Force quit on macOS (Super modifier = Cmd key)
  if ((modifierMask & kSensitiveCmdQ) != 0 && keyCode == kKeyQ) {
    LOG_WARN("blocked sensitive combination: Cmd+Q");
    return true;
  }

  // Ctrl+Alt+Backspace — X server kill on Linux
  if ((modifierMask & kSensitiveCtrlAltBksp) == kSensitiveCtrlAltBksp &&
      keyCode == kKeyBackspace) {
    LOG_WARN("blocked sensitive combination: Ctrl+Alt+Backspace");
    return true;
  }

  return false;
}

bool InputValidator::isRateLimited(
    uint32_t keyCode, std::chrono::steady_clock::time_point now)
{
  auto &timestamps = m_eventTimestamps[keyCode];

  // Clean up entries older than 1 second (sliding window)
  auto cutoff = now - std::chrono::seconds(1);
  timestamps.erase(
      std::remove_if(
          timestamps.begin(), timestamps.end(),
          [cutoff](const auto &t) { return t < cutoff; }),
      timestamps.end());

  // Check if rate limit exceeded
  if (timestamps.size() >= m_maxEventsPerSecond) {
    LOG_WARN("rate limited key code 0x%04x: %zu events in window",
             keyCode, timestamps.size());
    return true;
  }

  // Record this event timestamp
  timestamps.push_back(now);
  return false;
}

void InputValidator::setMaxEventsPerSecond(uint32_t max)
{
  m_maxEventsPerSecond = (max > 0) ? max : 1000;
}

uint32_t InputValidator::maxEventsPerSecond() const
{
  return m_maxEventsPerSecond;
}
