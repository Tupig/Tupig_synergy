/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 TuPig
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "FileTransferPath.h"

#include <algorithm>
#include <array>
#include <cctype>

namespace {

//! Longest name most filesystems accept; beyond this the write fails anyway.
constexpr size_t kMaxFileNameLength = 255;

//! Characters Windows forbids in a file name even without a path separator.
constexpr std::string_view kForbiddenCharacters = "<>:\"|?*";

std::string toLowerAscii(std::string_view text)
{
  std::string lowered(text);
  std::transform(lowered.begin(), lowered.end(), lowered.begin(), [](unsigned char c) {
    return static_cast<char>(std::tolower(c));
  });
  return lowered;
}

//! Windows device names are reserved whatever the extension or case.
bool isReservedDeviceName(const std::string &loweredName)
{
  // Compare only the stem, so "con.txt" is caught as well as "con".
  const auto dot = loweredName.find('.');
  const auto stem = loweredName.substr(0, dot);

  constexpr std::array<std::string_view, 22> kReserved = {"con",  "prn",  "aux",  "nul",  "com1", "com2",
                                                          "com3", "com4", "com5", "com6", "com7", "com8",
                                                          "com9", "lpt1", "lpt2", "lpt3", "lpt4", "lpt5",
                                                          "lpt6", "lpt7", "lpt8", "lpt9"};

  return std::find(kReserved.begin(), kReserved.end(), stem) != kReserved.end();
}

bool hasControlCharacter(std::string_view name)
{
  return std::any_of(name.begin(), name.end(), [](unsigned char c) {
    // NUL terminates a path in the C API, and the rest are never legitimate.
    return c < 0x20 || c == 0x7F;
  });
}

} // namespace

std::string FileTransferPath::sanitizeFileName(std::string_view raw)
{
  if (raw.empty() || raw.size() > kMaxFileNameLength) {
    return {};
  }

  if (hasControlCharacter(raw)) {
    return {};
  }

  // Any separator or drive letter means the peer is trying to steer the write
  // somewhere other than the drop directory. Refuse rather than strip: stripping
  // silently turns "../../x" into "x", which hides an attack attempt.
  if (raw.find('/') != std::string_view::npos || raw.find('\\') != std::string_view::npos) {
    return {};
  }

  if (raw.find(':') != std::string_view::npos) {
    return {};
  }

  if (std::any_of(raw.begin(), raw.end(), [](char c) {
        return kForbiddenCharacters.find(c) != std::string_view::npos;
      })) {
    return {};
  }

  if (raw == "." || raw == "..") {
    return {};
  }

  // Windows silently drops a trailing dot or space, which means "evil." and
  // "evil" would collide after the fact. Reject so what we check is what lands.
  if (raw.back() == '.' || raw.back() == ' ') {
    return {};
  }

  const std::string name(raw);
  if (isReservedDeviceName(toLowerAscii(name))) {
    return {};
  }

  return name;
}

std::string FileTransferPath::joinUnderDirectory(const std::string &directory, const std::string &safeName)
{
  if (directory.empty()) {
    return safeName;
  }

  const bool hasSeparator = directory.back() == '/' || directory.back() == '\\';
  return hasSeparator ? directory + safeName : directory + "/" + safeName;
}

std::string FileTransferPath::joinNames(const std::vector<std::string> &names)
{
  // Each name is written NUL-terminated, so the payload ends with a separator.
  // Note this is not a perfect round-trip: splitNames() drops empty segments, so
  // an empty name disappears. That is deliberate - an empty segment is never a
  // file to act on - but it means a caller must not derive the announced count by
  // splitting the payload it just built, or the two can disagree by the number of
  // empty names.
  std::string payload;
  for (const auto &name : names) {
    payload.append(name);
    payload.push_back('\0');
  }
  return payload;
}

std::vector<std::string> FileTransferPath::splitNames(std::string_view payload)
{
  std::vector<std::string> names;

  size_t start = 0;
  while (start < payload.size()) {
    const auto end = payload.find('\0', start);
    const auto segment = payload.substr(start, (end == std::string_view::npos ? payload.size() : end) - start);

    if (!segment.empty()) {
      names.emplace_back(segment);
    }

    if (end == std::string_view::npos) {
      break;
    }
    start = end + 1;
  }

  return names;
}
