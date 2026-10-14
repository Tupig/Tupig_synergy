/*
 * Synergy -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Chris Rizzitello <https://github.com/Tupig/Tupig_synergy/issues>
 * SPDX-FileCopyrightText: (C) 2015 - 2016 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "IKeyStateTests.h"

#include "IKeyState.h"

#include <memory>
#include <set>
#include <string>

void IKeyStateTests::allocDestination()
{
  auto info = IKeyState::KeyInfo::alloc(1, 2, 3, 4, {"test1", "test2"});
  QCOMPARE(info->m_screens, ":test1:test2:");
}

void IKeyStateTests::allocCopyPreservesFields()
{
  auto original = std::unique_ptr<IKeyState::KeyInfo>(IKeyState::KeyInfo::alloc(0x10, 0x20, 0x30, 2, {"a", "b"}));
  auto copy = std::unique_ptr<IKeyState::KeyInfo>(IKeyState::KeyInfo::alloc(*original));

  QVERIFY(IKeyState::KeyInfo::equal(original.get(), copy.get()));
  QCOMPARE(copy->m_key, 0x10);
  QCOMPARE(copy->m_mask, static_cast<KeyModifierMask>(0x20));
  QCOMPARE(copy->m_button, 0x30);
  QCOMPARE(copy->m_count, 2);
  QCOMPARE(copy->m_screens, std::string(":a:b:"));

  copy->m_count = 3;
  QVERIFY(!IKeyState::KeyInfo::equal(original.get(), copy.get()));
}

void IKeyStateTests::isDefaultDetectsUnsetLists()
{
  QVERIFY(IKeyState::KeyInfo::isDefault(nullptr));
  QVERIFY(IKeyState::KeyInfo::isDefault(""));
  QVERIFY(!IKeyState::KeyInfo::isDefault(":screen:"));
  QVERIFY(!IKeyState::KeyInfo::isDefault("*"));
}

void IKeyStateTests::containsMatchesWholeNames()
{
  // an unset/empty list matches nothing
  QVERIFY(!IKeyState::KeyInfo::contains("", "a"));
  QVERIFY(!IKeyState::KeyInfo::contains(nullptr, "a"));

  // the wildcard matches everything
  QVERIFY(IKeyState::KeyInfo::contains("*", "a"));

  // names are surrounded by ':' so partial names must not match
  QVERIFY(IKeyState::KeyInfo::contains(":a:bc:", "a"));
  QVERIFY(IKeyState::KeyInfo::contains(":a:bc:", "bc"));
  QVERIFY(!IKeyState::KeyInfo::contains(":ab:c:", "b"));
  QVERIFY(!IKeyState::KeyInfo::contains(":a:bc:", "c"));
}

void IKeyStateTests::joinAndSplitRoundTrip()
{
  QCOMPARE(IKeyState::KeyInfo::join({}), std::string());
  QCOMPARE(IKeyState::KeyInfo::join({"a", "b"}), std::string(":a:b:"));

  // the wildcard collapses the whole list
  QCOMPARE(IKeyState::KeyInfo::join({"a", "*", "b"}), std::string("*"));

  std::set<std::string> split;
  IKeyState::KeyInfo::split(":a:b:", split);
  QVERIFY(split == (std::set<std::string>{"a", "b"}));

  // a malformed list without the trailing separator still yields the last name
  IKeyState::KeyInfo::split(":a:b", split);
  QVERIFY(split == (std::set<std::string>{"a", "b"}));

  IKeyState::KeyInfo::split("*", split);
  QVERIFY(split == (std::set<std::string>{"*"}));

  IKeyState::KeyInfo::split("", split);
  QVERIFY(split.empty());
}

QTEST_MAIN(IKeyStateTests)
