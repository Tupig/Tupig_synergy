/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 TuPig
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "FileTransferReceiver.h"

#include "FileTransferPath.h"
#include "ProtocolUtil.h"
#include "base/Log.h"
#include "io/IStream.h"

#include <filesystem>
#include <fstream>
#include <system_error>

namespace {

//! How many `name (n).ext` variants to try before giving up on a collision.
constexpr int kMaxCollisionAttempts = 100;

//! Split a name into stem and extension so a collision suffix stays before the dot.
std::pair<std::string, std::string> splitStemAndExtension(const std::string &name)
{
  const auto dot = name.rfind('.');
  // A leading dot is part of the stem, not an extension separator.
  if (dot == std::string::npos || dot == 0) {
    return {name, {}};
  }
  return {name.substr(0, dot), name.substr(dot)};
}

//! True when \p candidate, once normalised, still lives inside \p directory.
/*!
Belt and braces after sanitizeFileName(). It should be impossible to fail, which
is exactly why it is cheap to assert: if sanitisation ever regresses, the write is
still refused rather than escaping the directory.
*/
bool staysInside(const std::filesystem::path &directory, const std::filesystem::path &candidate)
{
  std::error_code ec;
  const auto base = std::filesystem::weakly_canonical(directory, ec);
  if (ec) {
    return false;
  }

  const auto parent = std::filesystem::weakly_canonical(candidate.parent_path(), ec);
  if (ec) {
    return false;
  }

  const auto baseText = base.generic_string();
  const auto parentText = parent.generic_string();

  if (parentText.size() < baseText.size()) {
    return false;
  }
  if (parentText.compare(0, baseText.size(), baseText) != 0) {
    return false;
  }

  // Guard against a sibling directory whose name merely starts with the same text
  // (e.g. base "/tmp/drop" vs parent "/tmp/dropped").
  return parentText.size() == baseText.size() || parentText[baseText.size()] == '/' || baseText.back() == '/';
}

} // namespace

FileTransferReceiver::FileTransferReceiver(Options options) : m_options(std::move(options))
{
  if (m_options.dropDirectory.empty()) {
    LOG_WARN("file transfer: no drop directory configured, every transfer will be refused");
  }
}

bool FileTransferReceiver::onDragInfo(deskflow::IStream *stream)
{
  // A new announce starts a new drag: drop anything left from the previous one.
  reset();

  // Parse first, always. The message body must be consumed whether or not this
  // side acts on it - leaving the payload in the stream would desynchronise every
  // later message, which is far worse than ignoring a transfer.
  uint32_t announcedCount = 0;
  std::string payload;
  if (!ProtocolUtil::readf(stream, kMsgDDragInfo + 4, &announcedCount, &payload)) {
    LOG_ERR("file transfer: unreadable drag announce");
    return false;
  }

  if (!m_options.enabled) {
    LOG_DEBUG("file transfer: disabled, ignoring drag announce");
    return false;
  }

  // With no directory configured there is nowhere safe to put the files. Guarding
  // here as well as at write time keeps the two from drifting - and an empty path
  // joined with a name yields a RELATIVE path, which would silently land in the
  // process working directory.
  if (m_options.dropDirectory.empty()) {
    LOG_ERR("file transfer: no drop directory configured, refusing the whole drag");
    return false;
  }

  auto names = FileTransferPath::splitNames(payload);

  // Trust the name list over the count field; they disagree only for a malformed
  // or hostile sender, and the list is what actually gets written.
  if (names.empty()) {
    LOG_ERR("file transfer: drag announced no usable names");
    return false;
  }

  if (names.size() > m_options.maxFileCount) {
    LOG_ERR("file transfer: drag of %zu files exceeds the limit of %zu", names.size(), m_options.maxFileCount);
    m_refused += names.size();
    return false;
  }

  for (const auto &raw : names) {
    const auto safe = FileTransferPath::sanitizeFileName(raw);
    if (safe.empty()) {
      // Refuse outright: writing under a substitute name would put a file
      // somewhere the peer never asked for and hide the attempt.
      LOG_WARN("file transfer: refused unsafe file name from peer");
      ++m_refused;
      continue;
    }
    m_acceptedNames.push_back(safe);
  }

  if (m_acceptedNames.empty()) {
    LOG_ERR("file transfer: every announced name was refused");
    return false;
  }

  LOG_DEBUG("file transfer: drag accepted with %zu usable name(s)", m_acceptedNames.size());
  return true;
}

TransferState FileTransferReceiver::onFileChunk(deskflow::IStream *stream)
{
  // assemble() reads the message body, so it runs even when disabled: the body has
  // to come out of the stream either way or the next message is read as garbage.
  const auto result = FileChunk::assemble(stream, m_buffer, m_state, m_options.maxFileSize);

  if (!m_options.enabled) {
    m_buffer.clear();
    m_buffer.shrink_to_fit();
    return TransferState::Error;
  }

  switch (result) {
  case TransferState::Started:
  case TransferState::InProgress:
    return result;

  case TransferState::Finished:
    writeCurrentFile();
    return result;

  case TransferState::Error:
  default:
    ++m_refused;
    // assemble() already reset its own state; drop the buffer too so a failed
    // transfer cannot leak into the next one.
    m_buffer.clear();
    m_buffer.shrink_to_fit();
    return TransferState::Error;
  }
}

void FileTransferReceiver::reset()
{
  m_acceptedNames.clear();
  m_nextName = 0;
  m_buffer.clear();
  m_state = {};
}

void FileTransferReceiver::writeCurrentFile()
{
  const auto content = std::move(m_buffer);
  m_buffer.clear();

  // Second guard, alongside the one in onDragInfo: writing with an empty drop
  // directory would resolve to a relative path and escape into the working
  // directory rather than fail.
  if (m_options.dropDirectory.empty()) {
    LOG_ERR("file transfer: no drop directory configured, discarding received content");
    ++m_refused;
    return;
  }

  if (m_nextName >= m_acceptedNames.size()) {
    // More transfers than announced names: the peer is not following the protocol.
    LOG_ERR("file transfer: received a file with no announced name");
    ++m_refused;
    return;
  }

  if (m_totalWritten + content.size() > m_options.maxTotalBytes) {
    LOG_ERR(
        "file transfer: total of %llu bytes would exceed the limit of %llu",
        static_cast<unsigned long long>(m_totalWritten + content.size()),
        static_cast<unsigned long long>(m_options.maxTotalBytes)
    );
    ++m_refused;
    return;
  }

  const auto &safeName = m_acceptedNames.at(m_nextName);
  const auto target = uniqueTargetPath(safeName);
  if (target.empty()) {
    LOG_ERR("file transfer: no free target path for an accepted name");
    ++m_refused;
    return;
  }

  if (!staysInside(m_options.dropDirectory, target)) {
    LOG_ERR("file transfer: refusing to write outside the drop directory");
    ++m_refused;
    return;
  }

  std::error_code ec;
  std::filesystem::create_directories(m_options.dropDirectory, ec);

  // Stage then rename: a partial transfer must never appear as a finished file.
  const auto staging = std::filesystem::path(target + ".part");

  {
    std::ofstream out(staging, std::ios::binary | std::ios::trunc);
    if (!out) {
      LOG_ERR("file transfer: cannot open a staging file in the drop directory");
      ++m_refused;
      return;
    }

    out.write(content.data(), static_cast<std::streamsize>(content.size()));
    if (!out) {
      LOG_ERR("file transfer: failed while writing the staging file");
      out.close();
      std::filesystem::remove(staging, ec);
      ++m_refused;
      return;
    }
  }

  std::filesystem::rename(staging, target, ec);
  if (ec) {
    LOG_ERR("file transfer: cannot move the staged file into place: %s", ec.message().c_str());
    std::filesystem::remove(staging, ec);
    ++m_refused;
    return;
  }

  m_totalWritten += content.size();
  ++m_nextName;
  m_writtenFiles.push_back(target);
  LOG_INFO("file transfer: wrote %s (%zu bytes)", target.c_str(), content.size());
}

std::string FileTransferReceiver::uniqueTargetPath(const std::string &safeName) const
{
  const std::filesystem::path base(m_options.dropDirectory);
  const auto direct = base / safeName;

  std::error_code ec;
  if (!std::filesystem::exists(direct, ec)) {
    return direct.string();
  }

  // Never overwrite: an incoming file must not be able to replace a local one.
  const auto [stem, extension] = splitStemAndExtension(safeName);
  for (int attempt = 1; attempt <= kMaxCollisionAttempts; ++attempt) {
    const auto candidate = base / (stem + " (" + std::to_string(attempt) + ")" + extension);
    if (!std::filesystem::exists(candidate, ec)) {
      return candidate.string();
    }
  }

  return {};
}
