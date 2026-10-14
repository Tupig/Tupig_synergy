/*
 * Synergy -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 TuPig Synergy Contributors
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "ConfigReadTests.h"

#include "server/Config.h"

#include <sstream>
#include <string>

using namespace synergy::server;

void ConfigReadTests::parsesScreensAndLinks()
{
  std::istringstream input(
      "section: screens\n"
      "  alpha:\n"
      "  beta:\n"
      "end\n"
      "\n"
      "section: links\n"
      "  alpha:\n"
      "    right = beta\n"
      "  beta:\n"
      "    left = alpha\n"
      "end\n"
  );

  Config config(nullptr);
  ConfigReadContext context(input);
  config.read(context);

  QVERIFY(config.isScreen("alpha"));
  QVERIFY(config.isScreen("beta"));

  QVERIFY(config.hasNeighbor("alpha", Direction::Right));
  float position = 0.0f;
  QCOMPARE(config.getNeighbor("alpha", Direction::Right, 0.5f, &position), std::string("beta"));
  QVERIFY(config.hasNeighbor("beta", Direction::Left));
  QVERIFY(!config.hasNeighbor("alpha", Direction::Left));
}

void ConfigReadTests::parsesGlobalOptions()
{
  std::istringstream input(
      "section: options\n"
      "  clipboardSharing = true\n"
      "end\n"
  );

  Config config(nullptr);
  ConfigReadContext context(input);
  config.read(context);

  const auto *options = config.getOptions("");
  QVERIFY(options != nullptr);
  const auto option = options->find(kOptionClipboardSharing);
  QVERIFY(option != options->end());
  QVERIFY(option->second != 0);
}

void ConfigReadTests::rejectsUnknownSectionWithLineNumber()
{
  std::istringstream input(
      "section: screens\n"
      "  alpha:\n"
      "end\n"
      "section: bogus\n"
      "end\n"
  );

  Config config(nullptr);
  ConfigReadContext context(input);

  bool threw = false;
  try {
    config.read(context);
  } catch (const ServerConfigReadException &e) {
    threw = true;
    const auto what = QString::fromStdString(e.what());
    QVERIFY2(what.contains(QStringLiteral("line 4")), qPrintable(what));
    QVERIFY2(what.contains(QStringLiteral("bogus")), qPrintable(what));
  }
  QVERIFY(threw);
}

void ConfigReadTests::rejectsDataOutsideSection()
{
  std::istringstream input("garbage\n");

  Config config(nullptr);
  ConfigReadContext context(input);

  bool threw = false;
  try {
    config.read(context);
  } catch (const ServerConfigReadException &e) {
    threw = true;
    const auto what = QString::fromStdString(e.what());
    QVERIFY2(what.contains(QStringLiteral("line 1")), qPrintable(what));
  }
  QVERIFY(threw);
}

QTEST_MAIN(ConfigReadTests)
