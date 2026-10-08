/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 TuPig
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "FileChunk.h"
#include "ProtocolTypes.h"

#include <cstdint>
#include <string>
#include <vector>

namespace deskflow {
class IStream;
}

//! Receives drag-and-drop files and writes them to a drop directory.
/*!
The peer is a trust boundary: everything here - file names, declared sizes, counts -
is attacker-controlled, and the outcome is bytes landing on the local disk. Every
check therefore fails closed, and the class refuses rather than guesses when a
value cannot be shown to be safe.

What it enforces:

- Names must survive FileTransferPath::sanitizeFileName(), which rejects traversal
  (`../`), absolute paths, drive letters, control characters, reserved device names
  and over-long names. A refused name is counted and skipped; its content is
  discarded rather than written under a substitute name.
- The announced file count and each declared size are checked before any content is
  accepted, so a peer cannot make the receiver buffer an arbitrary amount.
- A transfer that ends short, or with more bytes than declared, is discarded.
- The final path is re-checked to be inside the drop directory after normalisation,
  so sanitisation is not the only line of defence.
- Writes are staged to a temporary file and renamed into place, so a partial
  transfer never appears as a finished file, and an existing file is never
  overwritten - a unique name is chosen instead.
*/
class FileTransferReceiver
{
public:
  struct Options
  {
    //! Where accepted files are written. Empty refuses every transfer.
    std::string dropDirectory;

    //! Largest single file accepted.
    uint64_t maxFileSize = 64ull * 1024 * 1024;

    //! Largest number of files accepted in one drag.
    size_t maxFileCount = 32;

    //! Largest total written across all files in one drag.
    uint64_t maxTotalBytes = 256ull * 1024 * 1024;

    //! Test seam: skip the work when nothing is listening.
    bool enabled = true;
  };

  explicit FileTransferReceiver(Options options);

  //! Read and apply a `DDRG` body from \p stream.
  /*!
  The stream must be positioned just past the 4-byte message code.
  \return true if the announce was accepted
  */
  bool onDragInfo(deskflow::IStream *stream);

  //! Read and apply one `DFTR` body from \p stream.
  /*!
  The stream must be positioned just past the 4-byte message code.
  \return what happened; `Finished` means a file was written
  */
  TransferState onFileChunk(deskflow::IStream *stream);

  //! Discard any in-flight transfer and clear per-drag bookkeeping.
  void reset();

  //! Names announced and accepted, in order.
  const std::vector<std::string> &acceptedNames() const
  {
    return m_acceptedNames;
  }

  //! Absolute paths written so far.
  const std::vector<std::string> &writtenFiles() const
  {
    return m_writtenFiles;
  }

  //! How many items were refused since the last reset (names, over-limit content).
  size_t refusedCount() const
  {
    return m_refused;
  }

private:
  //! Write the buffered content under the next accepted name.
  void writeCurrentFile();

  //! Pick a free path in the drop directory, or empty if none can be found.
  std::string uniqueTargetPath(const std::string &safeName) const;

  Options m_options;

  std::vector<std::string> m_acceptedNames;
  std::vector<std::string> m_writtenFiles;

  //! Index of the name to use for the next completed transfer.
  size_t m_nextName = 0;

  size_t m_refused = 0;
  uint64_t m_totalWritten = 0;

  std::string m_buffer;
  FileTransferAssemblyState m_state;
};
