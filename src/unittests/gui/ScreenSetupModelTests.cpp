/*
 * Synergy -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 TuPig
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "ScreenSetupModelTests.h"

#include "gui/ScreenSetupModel.h"
#include "gui/config/Screen.h"
#include "gui/config/ScreenList.h"

void ScreenSetupModelTests::clampsRowsToScreenList()
{
  ScreenList screens;
  screens.append(Screen(QStringLiteral("a")));
  screens.append(Screen(QStringLiteral("b")));
  screens.append(Screen(QStringLiteral("c")));
  screens.append(Screen(QStringLiteral("d")));

  // a 3x9 grid cannot hold only 4 screens; rows clamp instead of aborting
  ScreenSetupModel model(screens, 3, 9);

  QCOMPARE(model.columnCount(), 3);
  QCOMPARE(model.rowCount(), 1);
  QVERIFY(model.rowCount() * model.columnCount() <= screens.size());
}

void ScreenSetupModelTests::clampsGridSize()
{
  ScreenList screens;
  screens.append(Screen(QStringLiteral("a")));

  ScreenSetupModel model(screens, 200, 200);

  QCOMPARE(model.columnCount(), 100);
  QVERIFY(model.rowCount() * model.columnCount() <= screens.size());
}

void ScreenSetupModelTests::outOfRangeIndexesAreInvalid()
{
  ScreenList screens;
  for (int i = 0; i < 9; ++i) {
    screens.append(Screen(QStringLiteral("screen%1").arg(i)));
  }

  ScreenSetupModel model(screens, 3, 3);

  QVERIFY(!model.data(model.index(3, 0)).isValid());
  QVERIFY(!model.data(model.index(0, 3)).isValid());
  QVERIFY(!model.data(model.index(-1, 0)).isValid());
}

void ScreenSetupModelTests::inRangeIndexesHaveNames()
{
  ScreenList screens;
  for (int i = 0; i < 9; ++i) {
    screens.append(Screen(QStringLiteral("screen%1").arg(i)));
  }

  ScreenSetupModel model(screens, 3, 3);

  QCOMPARE(model.data(model.index(2, 2)).toString(), QStringLiteral("screen8"));
}

QTEST_MAIN(ScreenSetupModelTests)
