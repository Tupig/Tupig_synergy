/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 TuPig
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "base/Log.h"

#include <QTest>

class FileChunkTests : public QObject
{
  Q_OBJECT
private Q_SLOTS:
  void initTestCase();
  void chunkSizeFitsTransportLimit();
  void sendRefusesPayloadAboveTransportLimit();
  void splitProducesNoChunksForEmptyInput();
  void splitProducesSingleChunkWhenUnderLimit();
  void splitHonoursChunkSizeAndPreservesBytes();
  void roundTripSingleChunkFile();
  void roundTripFileSpanningSeveralChunks();
  void roundTripEmptyFile();
  void assembleRejectsDataChunkBeforeStart();
  void assembleRejectsEndBeforeStart();
  void assembleRejectsSizeBeyondLimit();
  void assembleRejectsDataBeyondDeclaredSize();
  void assembleRejectsShortTransfer();
  void assembleRejectsRestartWhileActive();
  void assembleRejectsUnknownMark();
  void assembleDoesNotPreallocateDeclaredSize();

private:
  Log m_log;
};
