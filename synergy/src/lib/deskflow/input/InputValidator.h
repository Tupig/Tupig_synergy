/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "deskflow/input/KeyTypes.h"
#include "deskflow/input/MouseTypes.h"

#include <chrono>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

//! Validation of input events arriving from the network.
/*!
A client applies whatever input the server sends it, so the server is a trust
boundary: a compromised or spoofed server can push crafted keyboard and mouse
events into the local platform layer. This class holds the checks applied to
those events at the client's inbound path (see ServerProxy).

Scope, and why it is narrower than it first appears:

- Key and button *range* checks are structurally redundant. The wire format
  carries a 16-bit key id and an 8-bit button id, so the storage types already
  bound the range. The values that are easy to mistake for "out of range" are in
  fact legal: kKeyNone is 0, and the X11 scroll wheel uses button ids 254 and
  255 (see MouseTypes.h). Rejecting on a hand-written range is therefore more
  likely to break real input than to stop an attack, so this class does not do it.

- Modifier mask validation is meaningful: the set of defined bits is closed and
  known (KeyTypes.h), so unknown bits genuinely indicate a malformed event.

- Rate limiting is meaningful: it bounds how much input a hostile peer can force
  the local machine to synthesise.

- Combination blocking is opt-in and empty by default. Nothing is intercepted
  unless the user lists it, because the interesting combinations are exactly the
  ones the OS needs to see: blocking the secure attention sequence would stop UAC
  prompts and the login screen from responding at all.
*/
class InputValidator
{
public:
  InputValidator() = default;

  //! A key together with the modifiers that must accompany it.
  struct KeyCombination
  {
    KeyID key = kKeyNone;

    /*! Modifiers that must all be held for the combination to match.
    A mask of 0 matches the key whatever modifiers are held, which is how a
    caller asks for the key to be blocked outright.
    */
    KeyModifierMask mask = 0;
  };

  //! Check a modifier mask contains only defined bits.
  /*!
  The defined bits are exactly those declared in KeyTypes.h. Anything else
  cannot come from a well-behaved peer, so the event is malformed rather than
  merely unusual.
  \param mask the modifier mask to validate
  \return true if every set bit is a defined modifier
  */
  static bool isValidModifierMask(KeyModifierMask mask);

  //! Clear any bits outside the defined modifier set.
  /*!
  Used to make a malformed mask safe without discarding the event. Dropping the
  event instead would lose input, and dropping a key-up in particular would
  leave the local machine with a key stuck down.
  \param mask the modifier mask to sanitize
  \return the mask with undefined bits removed
  */
  static KeyModifierMask sanitizeModifierMask(KeyModifierMask mask);

  //! Parse `key[:mask]` entries into combinations.
  /*!
  Both numbers accept `0x` hex or decimal, e.g. `"0x43:0x05"`. A malformed entry
  is logged and skipped rather than aborting the whole list, and never causes
  anything to be blocked - the failure mode has to be "blocked nothing", not
  "blocked something unintended".
  \param entries raw config strings
  \return the entries that parsed and validated
  */
  static std::vector<KeyCombination> parseBlockedCombinations(const std::vector<std::string> &entries);

  //! Replace the blocked set. Empty (the default) blocks nothing.
  /*!
  \param combinations the combinations to intercept
  */
  void setBlockedCombinations(std::vector<KeyCombination> combinations);

  //! Check whether an event matches a configured blocked combination.
  /*!
  A combination matches when the key is equal and every modifier it lists is
  held. Extra modifiers that the user has no reason to enumerate - CapsLock and
  NumLock - do not prevent a match.
  \param key the key to test
  \param mask the modifiers held
  \return true if the event should be dropped
  */
  [[nodiscard]] bool isBlockedCombination(KeyID key, KeyModifierMask mask) const;

  //! Number of configured blocked combinations.
  [[nodiscard]] size_t blockedCombinationCount() const;

  //! Check whether a button id is one this protocol defines.
  /*!
  For diagnostics only - callers must not drop events based on this. Button ids
  beyond the documented set are accepted by the wire format and may legitimately
  come from devices with more buttons than we know about, so an unrecognised id
  is worth logging but not worth rejecting.
  \param buttonId the button id to inspect
  \return true if the id is one of the documented values
  */
  static bool isKnownButtonId(ButtonID buttonId);

  //! Check whether an event would exceed the rate limit.
  /*!
  Uses a sliding one-second window per key. Old timestamps are dropped as the
  window advances, so memory does not grow without bound.
  \param keyCode the key being checked
  \param now current timestamp
  \return true if the event should be dropped
  */
  bool isRateLimited(
      KeyID keyCode, std::chrono::steady_clock::time_point now
  );

  //! Set the maximum events per second per key.
  void setMaxEventsPerSecond(uint32_t max);

  //! Get the current maximum events per second setting.
  [[nodiscard]] uint32_t maxEventsPerSecond() const;

private:
  //! Generous by design: this bounds flooding, not fast typing or key repeat.
  uint32_t m_maxEventsPerSecond = 1000;

  //! Opt-in interception list; empty means nothing is blocked.
  std::vector<KeyCombination> m_blockedCombinations;

  //! Sliding window timestamps per key for rate limiting.
  std::unordered_map<KeyID, std::vector<std::chrono::steady_clock::time_point>>
      m_eventTimestamps;
};
