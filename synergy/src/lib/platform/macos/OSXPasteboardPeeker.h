/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 TuPig
 * SPDX-FileCopyrightText: (C) 2013 - 2016 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#import <CoreFoundation/CoreFoundation.h>

#if defined(__cplusplus)
extern "C"
{
#endif

  //! Copy the file paths currently on the drag pasteboard, if any.
  /*!
  Returns a CFArray of CFString paths, or NULL when nothing is being dragged.
  The caller owns the returned array and must CFRelease it.
  *
  Prefer this over the old getDraggedFileURL() helper: that packed paths into one
  CFString with embedded NULs, which CFString APIs truncate at the first NUL, so
  only the first path survived.
  */
  CFArrayRef copyDraggedFilePaths(void);

#if defined(__cplusplus)
}
#endif
