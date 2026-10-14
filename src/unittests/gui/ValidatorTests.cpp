/*
 * Synergy -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Synergy Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "ValidatorTests.h"

#include "gui/config/ScreenList.h"
#include "gui/validators/ComputerNameValidator.h"
#include "gui/validators/EmptyStringValidator.h"
#include "gui/validators/IpAddressValidator.h"
#include "gui/validators/LineEditValidator.h"
#include "gui/validators/ScreenDuplicationsValidator.h"
#include "gui/validators/SpacesValidator.h"
#include "gui/validators/ValidationError.h"

#include <QLineEdit>

#include <memory>

void ValidatorTests::emptyStringValidator()
{
  validators::EmptyStringValidator validator(QStringLiteral("cannot be empty"));

  QVERIFY(!validator.validate(QString()));
  QVERIFY(validator.validate(QStringLiteral("a")));
  QCOMPARE(validator.getMessage(), QStringLiteral("cannot be empty"));
}

void ValidatorTests::spacesValidator_data()
{
  QTest::addColumn<QString>("input");
  QTest::addColumn<bool>("expected");

  QTest::newRow("empty") << QString() << true;
  QTest::newRow("plain") << QStringLiteral("hostname") << true;
  QTest::newRow("inner space") << QStringLiteral("host name") << false;
  QTest::newRow("only space") << QStringLiteral(" ") << false;
}

void ValidatorTests::spacesValidator()
{
  QFETCH(QString, input);
  QFETCH(bool, expected);

  validators::SpacesValidator validator(QStringLiteral("no spaces"));

  QCOMPARE(validator.validate(input), expected);
}

void ValidatorTests::computerNameValidator_data()
{
  QTest::addColumn<QString>("input");
  QTest::addColumn<bool>("expected");

  QTest::newRow("empty is allowed here") << QString() << true;
  QTest::newRow("host") << QStringLiteral("host") << true;
  QTest::newRow("dots dashes underscores") << QStringLiteral("host.name-1_2") << true;
  QTest::newRow("case insensitive") << QStringLiteral("UPPER") << true;
  QTest::newRow("space") << QStringLiteral("host name") << false;
  QTest::newRow("punctuation") << QStringLiteral("host!") << false;
  QTest::newRow("non ascii") << QStringLiteral("\xE4\xB8\xBB\xE6\x9C\xBA") << false;
  QTest::newRow("255 characters") << QString(255, QLatin1Char('a')) << true;
  QTest::newRow("256 characters") << QString(256, QLatin1Char('a')) << false;
}

void ValidatorTests::computerNameValidator()
{
  QFETCH(QString, input);
  QFETCH(bool, expected);

  validators::ComputerNameValidator validator(QStringLiteral("invalid"));

  QCOMPARE(validator.validate(input), expected);
}

void ValidatorTests::ipAddressValidator_data()
{
  QTest::addColumn<QString>("input");
  QTest::addColumn<bool>("expected");

  // the validator rejects IP addresses because aliases and names must not
  // look like one
  QTest::newRow("ipv4") << QStringLiteral("10.0.0.5") << false;
  QTest::newRow("ipv4 max") << QStringLiteral("255.255.255.255") << false;
  QTest::newRow("hostname") << QStringLiteral("host") << true;
  QTest::newRow("dotted name") << QStringLiteral("host.name") << true;
  QTest::newRow("incomplete ipv4") << QStringLiteral("1.2.3") << true;
}

void ValidatorTests::ipAddressValidator()
{
  QFETCH(QString, input);
  QFETCH(bool, expected);

  validators::IpAddressValidator validator(QStringLiteral("may not be an ip address"));

  QCOMPARE(validator.validate(input), expected);
}

void ValidatorTests::screenDuplicationsValidator()
{
  ScreenList screens;
  screens.append(Screen(QStringLiteral("beta")));
  screens.append(Screen(QStringLiteral("gamma")));
  screens.append(Screen());

  Screen server(QStringLiteral("delta"));
  server.markAsServer();
  screens.append(server);

  validators::ScreenDuplicationsValidator validator(QStringLiteral("duplicate"), QStringLiteral("alpha"), &screens);

  // the default name is always allowed, even when it appears in the list
  QVERIFY(validator.validate(QStringLiteral("alpha")));
  // a client screen with the same name is a duplicate
  QVERIFY(!validator.validate(QStringLiteral("beta")));
  // server screens are skipped by the check
  QVERIFY(validator.validate(QStringLiteral("delta")));
  // a fresh name is fine
  QVERIFY(validator.validate(QStringLiteral("new")));

  // without a screen list nothing can be a duplicate
  validators::ScreenDuplicationsValidator noList(QStringLiteral("duplicate"), QStringLiteral("alpha"), nullptr);
  QVERIFY(noList.validate(QStringLiteral("beta")));
}

void ValidatorTests::lineEditValidator_reportsFirstFailure()
{
  QLineEdit lineEdit;
  validators::ValidationError error(nullptr);
  validators::LineEditValidator validator(&lineEdit, &error);
  validator.addValidator(std::make_unique<validators::EmptyStringValidator>(QStringLiteral("cannot be empty")));
  validator.addValidator(std::make_unique<validators::SpacesValidator>(QStringLiteral("no spaces")));

  int pos = 0;

  // the first failing validator decides the message
  QString input = QStringLiteral("a b");
  QCOMPARE(validator.validate(input, pos), QValidator::Intermediate);
  QCOMPARE(error.message(), QStringLiteral("no spaces"));
  QVERIFY(!lineEdit.styleSheet().isEmpty());

  input = QString();
  QCOMPARE(validator.validate(input, pos), QValidator::Intermediate);
  QCOMPARE(error.message(), QStringLiteral("cannot be empty"));

  // a valid input clears the error and the highlight
  input = QStringLiteral("ab");
  QCOMPARE(validator.validate(input, pos), QValidator::Acceptable);
  QVERIFY(error.message().isEmpty());
  QVERIFY(lineEdit.styleSheet().isEmpty());
}

QTEST_MAIN(ValidatorTests)
