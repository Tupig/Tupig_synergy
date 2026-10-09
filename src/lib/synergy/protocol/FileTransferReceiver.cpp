/*
 * Synergy -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 TuPig
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "FileTransferReceiver.h"

#include "FileTransferPath.h"
#include "ProtocolUtil.h"
#include "base/Log.h"
#include "io/IStream.h"

#include <cerrno>
#include <cstdio>
#include <filesystem>
#include <system_error>

#if !defined(_WIN32)
#include <unistd.h>
#endif

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

//! Open \p path for writing only if it does not already exist.
FILE *openExclusive(const std::filesystem::path &path)
{
#if defined(_WIN32)
  return _wfopen(path.c_str(), L"wbx");
#else
  return std::fopen(path.c_str(), "wbx");
#endif
}

//! Publish a finished staging file as \p target without replacing anything.
/*!
std::filesystem::rename replaces an existing destination on POSIX. link() fails
with EEXIST instead, which is the rule this receiver promises. Filesystems that
cannot hard-link fall back to copy_file, which also errors when the destination
exists.
*/
bool publishStagedFile(const std::filesystem::path &staging, const std::filesystem::path &target, std::error_code &ec)
{
#if defined(_WIN32)
  std::filesystem::rename(staging, target, ec);
  return !ec;
#else
  if (::link(staging.c_str(), target.c_str()) == 0) {
    std::filesystem::remove(staging, ec);
    ec.clear();
    return true;
  }

  if (errno == EEXIST) {
    ec = std::error_code(errno, std::generic_category());
    return false;
  }

  std::filesystem::copy_file(staging, target, std::filesystem::copy_options::none, ec);
  std::error_code removeEc;
  std::filesystem::remove(staging, removeEc);
  return !ec;
#endif
}

//! True when the path does not exist. An error is treated as "occupied": guessing
//! that a probe failure means the name is free is how an overwrite slips through.
bool pathIsFree(const std::filesystem::path &path)
{
  std::error_code ec;
  const auto status = std::filesystem::status(path, ec);
  // MSVC reports not-found as an error code. That is "free", not "occupied".
  // Any other error means we cannot tell, so the name is treated as taken.
  if (status.type() == std::filesystem::file_type::not_found) {
    return true;
  }
  if (ec) {
    return false;
  }
  return !std::filesystem::exists(status);
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

bool FileTransferReceiver::onDragInfo(synergy::IStream *stream)
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

TransferState FileTransferReceiver::onFileChunk(synergy::IStream *stream)
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

  // The Nth completed transfer is paired with the Nth accepted name. A failed
  // write still consumes that name, otherwise the next file is stored under it.
  const auto safeName = m_acceptedNames.at(m_nextName);
  ++m_nextName;

  if (m_totalWritten + content.size() > m_options.maxTotalBytes) {
    LOG_ERR(
        "file transfer: total of %llu bytes would exceed the limit of %llu",
        static_cast<unsigned long long>(m_totalWritten + content.size()),
        static_cast<unsigned long long>(m_options.maxTotalBytes)
    );
    ++m_refused;
    return;
  }

  std::error_code ec;
  std::filesystem::create_directories(m_options.dropDirectory, ec);
  if (ec) {
    LOG_ERR("file transfer: cannot create the drop directory: %s", ec.message().c_str());
    ++m_refused;
    return;
  }

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

  // Stage exclusively, then publish without replacing an existing file.
  std::filesystem::path staging(target + ".part");
  FILE *staged = openExclusive(staging);
  if (staged == nullptr) {
    for (int attempt = 1; staged == nullptr && attempt <= 20; ++attempt) {
      staging = std::filesystem::path(target + "." + std::to_string(attempt) + ".part");
      staged = openExclusive(staging);
    }
  }
  if (staged == nullptr) {
    LOG_ERR("file transfer: cannot open a staging file in the drop directory");
    ++m_refused;
    return;
  }

  bool wrote = true;
  if (!content.empty()) {
    const auto n = std::fwrite(content.data(), 1, content.size(), staged);
    wrote = n == content.size();
  }
  wrote = wrote && std::fflush(staged) == 0;
  std::fclose(staged);
  if (!wrote) {
    LOG_ERR("file transfer: failed while writing the staging file");
    std::filesystem::remove(staging, ec);
    ++m_refused;
    return;
  }

  if (!publishStagedFile(staging, std::filesystem::path(target), ec)) {
    LOG_ERR("file transfer: cannot move the staged file into place: %s", ec.message().c_str());
    std::filesystem::remove(staging, ec);
    ++m_refused;
    return;
  }

  m_totalWritten += content.size();
  m_writtenFiles.push_back(target);
  LOG_INFO("file transfer: wrote %s (%zu bytes)", target.c_str(), content.size());
}

std::string FileTransferReceiver::uniqueTargetPath(const std::string &safeName) const
{
  const std::filesystem::path base(m_options.dropDirectory);
  const auto direct = base / safeName;

  if (pathIsFree(direct)) {
    return direct.string();
  }

  // Never overwrite: an incoming file must not be able to replace a local one.
  // A probe error counts as occupied, so we do not guess that the name is free.
  const auto [stem, extension] = splitStemAndExtension(safeName);
  for (int attempt = 1; attempt <= kMaxCollisionAttempts; ++attempt) {
    const auto candidate = base / (stem + " (" + std::to_string(attempt) + ")" + extension);
    if (pathIsFree(candidate)) {
      return candidate.string();
    }
  }

  return {};
}
