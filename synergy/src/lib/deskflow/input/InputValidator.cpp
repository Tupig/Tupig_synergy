/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "deskflow/input/InputValidator.h"

#include "base/Log.h"

#include <algorithm>

namespace {
//! Every bit the protocol can legitimately set.
/*!
Taken from KeyTypes.h rather than restated as literals, so this cannot drift
away from the wire format the way the previous hand-written masks had.
*/
constexpr KeyModifierMask kDefinedModifiers = //
    KeyModifierShift | KeyModifierControl | KeyModifierAlt | KeyModifierMeta |
    KeyModifierSuper | KeyModifierAltGr | KeyModifierLevel5Lock | KeyModifierCapsLock;
} // namespace

bool InputValidator::isValidModifierMask(KeyModifierMask mask)
{
  return (mask & ~kDefinedModifiers) == 0;
}

KeyModifierMask InputValidator::sanitizeModifierMask(KeyModifierMask mask)
{
  return mask & kDefinedModifiers;
}

bool InputValidator::isKnownButtonId(ButtonID buttonId)
{
  // Documented ids only; see MouseTypes.h. Note kButtonNone and the X11 scroll
  // wheel ids (254/255) are all legitimate, which is why this is not a range.
  switch (buttonId) {
  case kButtonNone:
  case kButtonLeft:
  case kButtonMiddle:
  case kButtonRight:
  case kButtonExtra0:
  case kButtonExtra1:
  case kX11ScrollWheelUp:
  case kX11ScrollWheelDown:
  case kX11ScrollWheelLeft:
  case kX11ScrollWheelRight:
    return true;
  default:
    return false;
  }
}

bool InputValidator::isRateLimited(
    KeyID keyCode, std::chrono::steady_clock::time_point now
)
{
  auto &timestamps = m_eventTimestamps[keyCode];

  // Drop everything that has fallen out of the one-second window.
  const auto cutoff = now - std::chrono::seconds(1);
  timestamps.erase(
      std::remove_if(
          timestamps.begin(), timestamps.end(),
          [cutoff](const auto &t) { return t < cutoff; }
      ),
      timestamps.end()
  );

  if (timestamps.size() >= m_maxEventsPerSecond) {
    LOG_WARN(
        "rate limited key 0x%04x: %zu events in window", keyCode, timestamps.size()
    );
    return true;
  }

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
