/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "deskflow/input/InputValidator.h"

#include "base/Log.h"

#include <algorithm>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

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

std::vector<InputValidator::KeyCombination> InputValidator::parseBlockedCombinations(const std::vector<std::string> &entries)
{
  std::vector<KeyCombination> combinations;

  for (const auto &entry : entries) {
    // Hand-edited config files carry stray spaces; tolerate them rather than
    // making the user match a byte-for-byte format.
    const auto first = entry.find_first_not_of(" \t");
    if (first == std::string::npos) {
      continue;
    }
    const auto last = entry.find_last_not_of(" \t");
    const auto trimmed = entry.substr(first, last - first + 1);

    const auto separator = trimmed.find(':');
    const auto keyText = trimmed.substr(0, separator);
    const auto maskText = (separator == std::string::npos) ? std::string("0") : trimmed.substr(separator + 1);

    // Fail safe: a malformed entry must never block something unintended, so it
    // is skipped instead of guessing. It must not abort the other entries either.
    try {
      size_t consumed = 0;

      const auto key = std::stoul(keyText, &consumed, 0);
      if (consumed != keyText.size()) {
        throw std::invalid_argument("trailing characters in key");
      }

      const auto mask = std::stoul(maskText, &consumed, 0);
      if (consumed != maskText.size()) {
        throw std::invalid_argument("trailing characters in mask");
      }

      // The wire format carries a 16-bit key id. Bound the mask to the bits
      // KeyTypes.h defines: anything else is a typo, and silently accepting it
      // would produce a combination that can never match.
      if (key > 0xFFFF) {
        throw std::out_of_range("key id does not fit the 16-bit wire field");
      }
      if (!isValidModifierMask(static_cast<KeyModifierMask>(mask))) {
        throw std::out_of_range("mask uses bits that are not modifiers");
      }

      combinations.push_back({static_cast<KeyID>(key), static_cast<KeyModifierMask>(mask)});
    } catch (const std::exception &) {
      LOG_WARN("ignoring unparsable blocked key combination \"%s\"", trimmed.c_str());
    }
  }

  return combinations;
}

void InputValidator::setBlockedCombinations(std::vector<KeyCombination> combinations)
{
  m_blockedCombinations = std::move(combinations);

  // Honour the setting, but say so: this is the failure mode that made the
  // previous unconditional block unusable, and it is silent from the user's side.
  for (const auto &combination : m_blockedCombinations) {
    const auto ctrlAlt = KeyModifierControl | KeyModifierAlt;
    if (combination.key == kKeyDelete && (combination.mask & ctrlAlt) == ctrlAlt) {
      LOG_WARN(
          "blocked combinations include Ctrl+Alt+Del; on Windows this stops UAC "
          "prompts and the login screen from responding"
      );
      break;
    }
  }
}

bool InputValidator::isBlockedCombination(KeyID key, KeyModifierMask mask) const
{
  for (const auto &combination : m_blockedCombinations) {
    if (combination.key != key) {
      continue;
    }

    // Require the listed modifiers to be held, but tolerate ones the user has no
    // reason to enumerate (CapsLock, NumLock); requiring an exact match would
    // make a combination stop working whenever a lock key happens to be on.
    if ((mask & combination.mask) == combination.mask) {
      return true;
    }
  }

  return false;
}

size_t InputValidator::blockedCombinationCount() const
{
  return m_blockedCombinations.size();
}

bool InputValidator::isKnownButtonId(ButtonID buttonId){
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
