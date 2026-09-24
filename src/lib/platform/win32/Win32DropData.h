/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 TuPig
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <cstddef>
#include <string>
#include <vector>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

namespace deskflow::win32 {

//! Read the file paths out of a `CF_HDROP` memory block.
/*!
Windows hands a dropped file list to a drop target as an `HGLOBAL` holding a
`DROPFILES` header followed by the paths: `pFiles` bytes into the block, a list of
NUL-terminated strings ended by an extra NUL. `fWide` says whether those strings
are UTF-16 (the normal case) or ANSI.
*
Keeping this separate from the OLE plumbing is what makes it testable: the input
is just bytes, so a unit test can synthesise a block - including malformed ones -
without a real drag or an `IDataObject`.
*
Deliberately stricter than the code it replaces, which was removed upstream in
5365e34f0. That version walked the list with `wcslen` (unbounded - a short or
hostile block reads past the end), never checked `GlobalLock`, and converted with
`wcstombs`, which is locale-dependent and so mangles any path outside the current
code page - non-ASCII file names arrived as mojibake.
*
\param data the locked block; must not be null
\param size the block's size in bytes, i.e. `GlobalSize()` of the handle
\return the paths found, in order. Empty if the block is too small to hold a
header, if `pFiles` points outside it, or if no complete path is present.
*/
std::vector<std::string> readDropFilePaths(const void *data, std::size_t size);

//! Build a wide (`fWide=TRUE`) `DROPFILES` block for the given UTF-8 paths.
/*!
Returns an empty vector if \p paths is empty or if any path cannot be converted
to UTF-16. The block is suitable for unit tests and for stuffing into an
`HGLOBAL` for `CF_HDROP`.
*/
std::vector<unsigned char> buildDropFileBlock(const std::vector<std::string> &utf8Paths);

//! Allocate a movable `HGLOBAL` holding `buildDropFileBlock(utf8Paths)`.
/*!
Caller owns the handle and must `GlobalFree` it (or hand it to OLE via
`STGMEDIUM`, which takes ownership on success). Returns `nullptr` on failure.
*/
HGLOBAL createDropFilesHGlobal(const std::vector<std::string> &utf8Paths);

//! Start an OLE drag of the given files (copy effect) under the current cursor.
/*!
Blocks until the user drops or cancels. Returns true if `DoDragDrop` reported
`DRAGDROP_S_DROP`. Requires COM/OLE already initialised on this thread
(as `MSWindowsScreen` does for `RegisterDragDrop`).
*/
bool startDraggingFiles(const std::vector<std::string> &utf8Paths);

} // namespace deskflow::win32
