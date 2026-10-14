/*
 * Synergy -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 TuPig
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "KeyMap.h"

#include <vector>

namespace synergy::input::detail {

//! Index of the entry requiring the fewest modifier changes.
/*!
Returns the index into \p entryList of the KeyItemList requiring the fewest
modifier changes between the current modifier state and \p desiredState, or
the index of an entry that can accommodate \p desiredState exactly.  Returns
-1 when no entry is suitable.

Extracted from KeyMap so the selection algorithm has a directly testable
entry point without granting test code access to private members.
*/
int32_t findBestKeyIndex(const std::vector<KeyMap::KeyItemList> &entryList, KeyModifierMask desiredState);

} // namespace synergy::input::detail
