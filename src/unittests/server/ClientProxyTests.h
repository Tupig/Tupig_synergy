/*
 * Synergy -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 TuPig Synergy Contributors
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <QTest>

class ClientProxyTests : public QObject
{
  Q_OBJECT
private Q_SLOTS:
  void handshakeNoopReturnsTrue();
  void handshakeUnknownCodeReturnsFalse();
  void handshakeInfoCompletesHandshake();
  void parseMessageNoopReturnsTrue();
  void parseMessageUnknownReturnsFalse();
  void parseMessageInfoUpdatesShape();
  void parseMessageTruncatedInfoReturnsFalse();
};
