/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 TuPig
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "FileTransferSource.h"

#include "FileChunk.h"
#include "FileTransferPath.h"
#include "base/Log.h"

#include <QFile>
#include <QFileInfo>

#include <utility>

namespace {

const char *toString(FileTransferSource::Status status)
{
  switch (status) {
  case FileTransferSource::Status::Ok:
    return "ok";
  case FileTransferSource::Status::NotFound:
    return "no such file";
  case FileTransferSource::Status::NotRegularFile:
    return "not a regular file";
  case FileTransferSource::Status::NotReadable:
    return "not readable";
  case FileTransferSource::Status::TooLarge:
    return "larger than the transfer limit";
  }
  return "unknown";
}

} // namespace

struct FileTransferSource::Impl
{
  QFile file;
};

FileTransferSource::FileTransferSource() : m_impl(std::make_unique<Impl>())
{
}

FileTransferSource::~FileTransferSource() = default;

FileTransferSource::Status FileTransferSource::inspect(
    const std::string &path, uint64_t maxFileSize, Entry &out
)
{
  const QFileInfo info(QString::fromStdString(path));

  if (!info.exists()) {
    return Status::NotFound;
  }

  // Directories, devices and sockets are not transferable as files.
  if (!info.isFile()) {
    return Status::NotRegularFile;
  }

  if (!info.isReadable()) {
    return Status::NotReadable;
  }

  const auto size = static_cast<uint64_t>(info.size());
  if (size > maxFileSize) {
    return Status::TooLarge;
  }

  out.path = path;
  out.name = info.fileName().toStdString();
  out.size = size;

  return Status::Ok;
}

std::vector<FileTransferSource::Entry> FileTransferSource::inspectAll(
    const std::vector<std::string> &paths, uint64_t maxFileSize,
    std::vector<std::pair<std::string, Status>> *rejected
)
{
  std::vector<Entry> entries;
  entries.reserve(paths.size());

  for (const auto &path : paths) {
    Entry entry;
    const auto status = inspect(path, maxFileSize, entry);

    if (status == Status::Ok) {
      entries.push_back(std::move(entry));
      continue;
    }

    // One bad member must not abort the selection, but it must be reported:
    // silently sending fewer files than the user dragged is worse than a log line.
    LOG_WARN("skipping \"%s\" for transfer: %s", path.c_str(), toString(status));
    if (rejected != nullptr) {
      rejected->emplace_back(path, status);
    }
  }

  return entries;
}

std::string FileTransferSource::dragInfoPayload(const std::vector<Entry> &entries)
{
  std::vector<std::string> names;
  names.reserve(entries.size());
  for (const auto &entry : entries) {
    names.push_back(entry.name);
  }

  // Reuse the one NUL-joining implementation rather than repeating it here, so
  // the sender and the receiver's splitter cannot drift apart.
  return FileTransferPath::joinNames(names);
}

FileTransferSource::Status FileTransferSource::open(Entry &entry)
{
  close();

  m_impl->file.setFileName(QString::fromStdString(entry.path));
  if (!m_impl->file.open(QIODevice::ReadOnly)) {
    LOG_ERR("cannot open \"%s\" for transfer: %s", entry.path.c_str(), m_impl->file.errorString().toStdString().c_str());
    return Status::NotReadable;
  }

  // Authoritative size: taken now, from the open handle, so the number the peer
  // is told to expect matches the bytes that will actually follow.
  const auto size = static_cast<uint64_t>(m_impl->file.size());
  entry.size = size;

  return Status::Ok;
}

std::string FileTransferSource::readChunk()
{
  if (!isOpen()) {
    return {};
  }

  const auto chunkSize = FileChunk::chunkSize();
  std::string chunk(chunkSize, '\0');

  const auto read = m_impl->file.read(chunk.data(), static_cast<qint64>(chunkSize));
  if (read < 0) {
    LOG_ERR("read error during transfer: %s", m_impl->file.errorString().toStdString().c_str());
    return {};
  }

  chunk.resize(static_cast<size_t>(read));
  return chunk;
}

uint64_t FileTransferSource::remaining() const
{
  if (!isOpen()) {
    return 0;
  }

  const auto size = static_cast<uint64_t>(m_impl->file.size());
  const auto position = static_cast<uint64_t>(m_impl->file.pos());
  return position >= size ? 0 : size - position;
}

bool FileTransferSource::isOpen() const
{
  return m_impl->file.isOpen();
}

void FileTransferSource::close()
{
  if (m_impl->file.isOpen()) {
    m_impl->file.close();
  }
}

uint64_t FileTransferSource::streamAll(const std::function<void(const std::string &)> &emit)
{
  uint64_t total = 0;

  while (true) {
    // Stop on the declared size rather than on end-of-file: if the file grew
    // since it was opened, sending the extra bytes would make the peer reject
    // the transfer for exceeding the size it was promised.
    const auto left = remaining();
    if (left == 0) {
      break;
    }

    auto chunk = readChunk();
    if (chunk.empty()) {
      break;
    }

    if (chunk.size() > left) {
      chunk.resize(static_cast<size_t>(left));
    }

    total += chunk.size();
    emit(chunk);
  }

  return total;
}
