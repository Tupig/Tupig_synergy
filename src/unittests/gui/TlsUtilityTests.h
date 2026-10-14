/*
 * Synergy -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Synergy Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include <QTest>

class TlsUtilityTests : public QObject
{
  Q_OBJECT
private Q_SLOTS:
  // Test are run in order top to bottom
  void initTestCase();
  void generateCertificateAt_createsValidCertificate();
  void isCertValid_rejectsMissingFile();
  void isCertValid_rejectsGarbageFile();
  void isCertValid_rejectsKeySizeMismatch();
  void getCertKeyLength_missingFile_returnsMinusOne();
  void certFingerprint_returnsSha256Digest();
  void certFingerprint_missingFile_isEmpty();
};
