/*
 * Synergy -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 TuPig
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "base/Log.h"

#include <QTest>

class PacketStreamFilterTests : public QObject
{
  Q_OBJECT
private Q_SLOTS:
  // Tests are run in order top to bottom
  void initTestCase();
  void splitFrame_reassemblesPayloadAcrossChunks();
  void oversizedHeader_marksFormatErrorAndDropsStream();
  void declaredLengthBeyondData_staysNotReady();
  void write_prependsFourByteLengthPrefix();

private:
  Log m_log;
};
