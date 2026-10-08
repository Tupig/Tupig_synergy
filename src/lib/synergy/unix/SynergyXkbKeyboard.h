/*
 * Synergy -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2021 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#if WINAPI_XWINDOWS

#pragma once

#include <X11/XKBlib.h>
#include <X11/extensions/XKBrules.h>

namespace synergy::linux {

class SynergyXkbKeyboard
{
  XkbRF_VarDefsRec m_data = {};

public:
  SynergyXkbKeyboard();
  SynergyXkbKeyboard(const SynergyXkbKeyboard &) = delete;
  SynergyXkbKeyboard &operator=(const SynergyXkbKeyboard &) = delete;

  const char *getLayout() const;
  const char *getVariant() const;

  ~SynergyXkbKeyboard();
};

} // namespace synergy::linux

#endif // WINAPI_XWINDOWS
