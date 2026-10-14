/*
 * Synergy -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 TuPig Synergy Contributors
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <QTest>

class NetworkAddressTests : public QObject
{
  Q_OBJECT
private Q_SLOTS:
  void parsesHostPortFromSingleColon();
  void parsesBracketedIpv6WithPort();
  void parsesUnbracketedLinkLocalWithScopeId();
  void keepsHostnameWhenNoPortSuffix();
  void rejectsEmptyPortAfterColon();
  void rejectsNonNumericPort();
  void rejectsPortOutOfRange();
  void rejectsMalformedBracketedIpv6();
  void rejectsLinkLocalWithoutScopeId();
  void unresolvedAddressIsInvalid();
};
