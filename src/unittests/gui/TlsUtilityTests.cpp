/*
 * Synergy -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Synergy Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "TlsUtilityTests.h"

#include "common/Settings.h"
#include "gui/TlsUtility.h"

#include <QFile>
#include <QTemporaryDir>

namespace {

constexpr int kTestKeyLength = 2048;

QString writeGarbageFile(QTemporaryDir &dir)
{
  const QString path = dir.filePath(QStringLiteral("garbage.pem"));
  QFile file(path);
  if (!file.open(QFile::WriteOnly)) {
    return QString();
  }
  file.write("this is not a certificate");
  return path;
}

} // namespace

void TlsUtilityTests::initTestCase()
{
  // isCertValid() compares the certificate key length against the setting
  Settings::setValue(Settings::Security::KeySize, kTestKeyLength);
}

void TlsUtilityTests::generateCertificateAt_createsValidCertificate()
{
  QTemporaryDir dir;
  QVERIFY(dir.isValid());
  // nested directory exercises the mkpath branch
  const QString certPath = dir.filePath(QStringLiteral("tls/certificate.pem"));

  QVERIFY(synergy::gui::TlsUtility::generateCertificateAt(certPath, kTestKeyLength));

  QVERIFY(QFile::exists(certPath));
  QVERIFY(synergy::gui::TlsUtility::isCertValid(certPath));
  QCOMPARE(synergy::gui::TlsUtility::getCertKeyLength(certPath), kTestKeyLength);
  QCOMPARE(synergy::gui::TlsUtility::certFingerprint(certPath).size(), 32);
}

void TlsUtilityTests::isCertValid_rejectsMissingFile()
{
  QVERIFY(!synergy::gui::TlsUtility::isCertValid(QStringLiteral("nonexistent/certificate.pem")));
}

void TlsUtilityTests::isCertValid_rejectsGarbageFile()
{
  QTemporaryDir dir;
  QVERIFY(dir.isValid());
  const QString path = writeGarbageFile(dir);
  QVERIFY(!path.isEmpty());

  QVERIFY(!synergy::gui::TlsUtility::isCertValid(path));
}

void TlsUtilityTests::isCertValid_rejectsKeySizeMismatch()
{
  QTemporaryDir dir;
  QVERIFY(dir.isValid());
  const QString certPath = dir.filePath(QStringLiteral("certificate.pem"));
  QVERIFY(synergy::gui::TlsUtility::generateCertificateAt(certPath, kTestKeyLength));

  Settings::setValue(Settings::Security::KeySize, kTestKeyLength * 2);
  QVERIFY(!synergy::gui::TlsUtility::isCertValid(certPath));
  Settings::setValue(Settings::Security::KeySize, kTestKeyLength);
  QVERIFY(synergy::gui::TlsUtility::isCertValid(certPath));
}

void TlsUtilityTests::getCertKeyLength_missingFile_returnsMinusOne()
{
  QCOMPARE(synergy::gui::TlsUtility::getCertKeyLength(QStringLiteral("nonexistent/certificate.pem")), -1);

  QTemporaryDir dir;
  QVERIFY(dir.isValid());
  const QString path = writeGarbageFile(dir);
  QVERIFY(!path.isEmpty());
  QCOMPARE(synergy::gui::TlsUtility::getCertKeyLength(path), -1);
}

void TlsUtilityTests::certFingerprint_returnsSha256Digest()
{
  QTemporaryDir dir;
  QVERIFY(dir.isValid());
  const QString certPath = dir.filePath(QStringLiteral("certificate.pem"));
  QVERIFY(synergy::gui::TlsUtility::generateCertificateAt(certPath, kTestKeyLength));

  const QByteArray fingerprint = synergy::gui::TlsUtility::certFingerprint(certPath);
  QCOMPARE(fingerprint.size(), 32);
  QVERIFY(!fingerprint.isEmpty());
}

void TlsUtilityTests::certFingerprint_missingFile_isEmpty()
{
  QVERIFY(synergy::gui::TlsUtility::certFingerprint(QStringLiteral("nonexistent/certificate.pem")).isEmpty());
}

QTEST_MAIN(TlsUtilityTests)
