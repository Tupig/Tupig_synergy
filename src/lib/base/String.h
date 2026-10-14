/*
 * Synergy -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <cstdarg>
#include <string>
#include <string_view>

//! std::string utilities
/*!
Provides functions for string manipulation.
*/
namespace synergy::string {

//! Format positional arguments
/*!
Format a string using positional arguments.  fmt has literal
characters and conversion specifications introduced by `\%':
- \%\%  -- literal `\%'
- \%{n} -- positional element n, n a positive integer, {} are literal

All arguments in the variable list are const char*.  Positional
elements are indexed from 1.
*/
std::string format(const char *fmt, ...);

//! Format positional arguments
/*!
Same as format() except takes va_list.
*/
std::string vformat(const char *fmt, va_list);

//! Print a string using sprintf-style formatting
/*!
Equivalent to sprintf() except the result is returned as a String.
*/
std::string sprintf(const char *fmt, ...);

//! Normalize line endings to LF
/*!
Converts CRLF pairs and lone CR characters to LF, so text from any platform
shares one canonical in-memory form.
*/
std::string convertLinefeedsToLf(std::string_view src);

//! Expand LF line endings to \p terminator
/*!
Replaces every LF with \p terminator (for example "\r\n" on Windows or "\r"
for the classic Mac OS convention).  The input is expected to be in the
canonical LF form produced by convertLinefeedsToLf().
*/
std::string convertLinefeedsFromLf(std::string_view src, std::string_view terminator);

//! Case-insensitive comparisons
/*!
This class provides case-insensitve comparison functions.
*/
class CaselessCmp
{
public:
  //! Same as less()
  bool operator()(const std::string &a, const std::string &b) const;

  //! Returns true iff \c a is lexicographically less than \c b
  static bool less(const std::string_view &a, const std::string_view &b);

  //! Returns true iff \c a is lexicographically equal to \c b
  static bool equal(const std::string &a, const std::string &b);

  //! Returns true iff \c a is lexicographically less than \c b
  static bool cmpLess(const std::string::value_type &a, const std::string::value_type &b);
};

} // namespace synergy::string
