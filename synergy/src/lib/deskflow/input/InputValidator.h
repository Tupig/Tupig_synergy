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
*/
class InputValidator
{
public:
  InputValidator() = default;

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

  //! Sliding window timestamps per key for rate limiting.
  std::unordered_map<KeyID, std::vector<std::chrono::steady_clock::time_point>>
      m_eventTimestamps;
};
