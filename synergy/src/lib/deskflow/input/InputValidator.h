/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <cstdint>
#include <chrono>
#include <unordered_map>
#include <vector>

//! Input event validator for security hardening.
/*!
Validates keyboard and mouse events received from the network before
they are forwarded to the local platform layer. Enforces:
- Range validation for key codes and button IDs
- Rate limiting to prevent flooding attacks
- Sensitive key combination interception
*/
class InputValidator
{
public:
  InputValidator() = default;

  //! Validate a key code is within acceptable range.
  /*!
  Key codes must be in range [1, 0xFFFF] to cover X11 keysyms,
  Windows virtual keys, and macOS key codes.
  \param keyCode the key code to validate
  \return true if the key code is valid
  */
  static bool isValidKeyCode(uint32_t keyCode);

  //! Validate a mouse button ID is within acceptable range.
  /*!
  Button IDs must be in range [1, 32] covering all standard mouse buttons.
  \param buttonId the button ID to validate
  \return true if the button ID is valid
  */
  static bool isValidButtonId(uint32_t buttonId);

  //! Validate a modifier mask has only defined bits set.
  /*!
  Modifier masks use only bits 0-15 for standard modifier keys.
  \param mask the modifier mask to validate
  \return true if the modifier mask is valid
  */
  static bool isValidModifierMask(uint32_t mask);

  //! Detect dangerous key combinations that should be blocked.
  /*!
  Checks for platform-dangerous combinations like:
  - Ctrl+Alt+Delete (Windows)
  - Cmd+Q (macOS)
  - Ctrl+Alt+Backspace (Linux X11)
  Uses raw modifier bitmasks without platform-specific headers.
  \param keyCode the key code
  \param modifierMask the current modifier state
  \return true if this combination should be blocked
  */
  static bool isSensitiveCombination(uint32_t keyCode, uint32_t modifierMask);

  //! Check if an event would exceed the rate limit.
  /*!
  Uses a sliding window per key code. Old entries are cleaned up
  automatically to prevent memory growth.
  \param keyCode the key code being checked
  \param now current timestamp
  \return true if the event should be dropped (rate limited)
  */
  bool isRateLimited(uint32_t keyCode,
                     std::chrono::steady_clock::time_point now);

  //! Set the maximum events per second per key code.
  void setMaxEventsPerSecond(uint32_t max);

  //! Get the current maximum events per second setting.
  [[nodiscard]] uint32_t maxEventsPerSecond() const;

private:
  uint32_t m_maxEventsPerSecond = 1000;

  // Sliding window timestamps per key code for rate limiting
  std::unordered_map<uint32_t, std::vector<std::chrono::steady_clock::time_point>>
      m_eventTimestamps;
};
