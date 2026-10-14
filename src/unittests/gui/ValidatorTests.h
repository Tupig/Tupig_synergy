/*
 * Synergy -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Synergy Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include <QTest>

class ValidatorTests : public QObject
{
  Q_OBJECT
private Q_SLOTS:
  // Test are run in order top to bottom
  void emptyStringValidator();
  void spacesValidator_data();
  void spacesValidator();
  void computerNameValidator_data();
  void computerNameValidator();
  void ipAddressValidator_data();
  void ipAddressValidator();
  void screenDuplicationsValidator();
  void lineEditValidator_reportsFirstFailure();
};
