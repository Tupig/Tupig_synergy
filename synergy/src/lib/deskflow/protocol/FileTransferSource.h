/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 TuPig
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

class QFile;

//! Reads local files and produces the chunk sequence for an outbound transfer.
/*!
This is the sending half of the `DDRG`/`DFTR` protocol, deliberately kept free of
the network, the event queue and the platform layer so the whole sequence can be
unit tested with ordinary temporary files.

The receiver verifies that the bytes it collects equal the size declared in the
`DFTR` start chunk (see FileChunk::assemble). That makes the declared size a
promise the sender must keep, so the size reported here is always taken from the
open handle rather than from an earlier stat: a file edited between the two would
otherwise be sent with a stale size and the peer would reject the whole transfer.
*/
class FileTransferSource
{
public:
  //! Why a candidate file cannot be sent.
  enum class Status
  {
    Ok,
    NotFound,
    NotRegularFile,
    NotReadable,
    TooLarge
  };

  //! A file selected for sending.
  struct Entry
  {
    //! Local path, never sent on the wire.
    std::string path;

    //! Base name, as advertised in the `DDRG` payload.
    std::string name;

    //! Size in bytes, authoritative once the file is open.
    uint64_t size = 0;
  };

  //! Inspect a candidate path without opening it for reading.
  /*!
  \param path local file to inspect
  \param maxFileSize largest file that may be sent
  \param out receives the entry when the result is Ok
  \return Ok, or the reason the file cannot be sent
  */
  static Status inspect(const std::string &path, uint64_t maxFileSize, Entry &out);

  //! Inspect a selection, keeping only the files that can be sent.
  /*!
  A selection is user-driven, so one unreadable member must not abort the rest;
  the rejected ones are reported through \p rejected when it is not null.
  \param paths candidate paths
  \param maxFileSize largest file that may be sent
  \param rejected receives paths that were dropped, with their reason
  \return the entries that can be sent, in the order given
  */
  static std::vector<Entry> inspectAll(
      const std::vector<std::string> &paths, uint64_t maxFileSize,
      std::vector<std::pair<std::string, Status>> *rejected = nullptr
  );

  //! Build the `DDRG` payload from a selection: NUL-separated base names.
  static std::string dragInfoPayload(const std::vector<Entry> &entries);

  FileTransferSource();
  ~FileTransferSource();

  FileTransferSource(const FileTransferSource &) = delete;
  FileTransferSource &operator=(const FileTransferSource &) = delete;

  //! Open \p entry for reading, refreshing its size from the open handle.
  /*!
  \param entry the file to send; its \c size is updated to the real size
  \return Ok, or the reason the file cannot be read
  */
  Status open(Entry &entry);

  //! Read the next chunk. Empty means the file is exhausted.
  std::string readChunk();

  //! Bytes not yet read.
  [[nodiscard]] uint64_t remaining() const;

  //! Whether a file is currently open.
  [[nodiscard]] bool isOpen() const;

  //! Close the current file, if any.
  void close();

  //! Read the whole file, handing every chunk to \p emit.
  /*!
  Exists so the chunk sequence can be exercised without a network: a test passes
  a lambda that appends to a buffer. Open the file first.
  \param emit called once per chunk, in order
  \return total bytes emitted
  */
  uint64_t streamAll(const std::function<void(const std::string &)> &emit);

private:
  struct Impl;
  std::unique_ptr<Impl> m_impl;
};
