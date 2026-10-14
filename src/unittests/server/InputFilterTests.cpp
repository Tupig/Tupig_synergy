/*
 * Synergy -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 TuPig Synergy Contributors
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "InputFilterTests.h"

#include "base/Event.h"
#include "base/EventTypes.h"
#include "server/InputFilter.h"

#include <string>

namespace {

//! Matches only System events, activating on them.
class SystemOnlyCondition : public InputFilter::Condition
{
public:
  Condition *clone() const override
  {
    return new SystemOnlyCondition();
  }
  std::string format() const override
  {
    return "system";
  }
  InputFilter::FilterStatus match(const Event &e) override
  {
    return e.getType() == EventTypes::System ? InputFilter::FilterStatus::Activate : InputFilter::FilterStatus::NoMatch;
  }
};

//! Activates on System events and deactivates on key-down events.
class ToggleCondition : public InputFilter::Condition
{
public:
  Condition *clone() const override
  {
    return new ToggleCondition();
  }
  std::string format() const override
  {
    return "toggle";
  }
  InputFilter::FilterStatus match(const Event &e) override
  {
    if (e.getType() == EventTypes::System) {
      return InputFilter::FilterStatus::Activate;
    }
    if (e.getType() == EventTypes::KeyStateKeyDown) {
      return InputFilter::FilterStatus::Deactivate;
    }
    return InputFilter::FilterStatus::NoMatch;
  }
};

//! Counts how often it is performed.
class CountingAction : public InputFilter::Action
{
public:
  explicit CountingAction(int *counter) : m_counter(counter)
  {
  }

  Action *clone() const override
  {
    return new CountingAction(m_counter);
  }
  std::string format() const override
  {
    return "count";
  }
  void perform(const Event &) override
  {
    ++(*m_counter);
  }

private:
  int *m_counter;
};

} // namespace

void InputFilterTests::initTestCase()
{
  m_log.setFilter(LogLevel::Level::Error);
}

void InputFilterTests::ruleWithoutConditionNeverMatches()
{
  InputFilter::Rule rule;
  QVERIFY(!rule.handleEvent(Event(EventTypes::System)));
  QVERIFY(!rule.handleEvent(Event(EventTypes::KeyStateKeyDown)));
}

void InputFilterTests::conditionMatchesOnlyItsEventType()
{
  InputFilter::Rule rule{new SystemOnlyCondition()};
  QVERIFY(rule.handleEvent(Event(EventTypes::System)));
  QVERIFY(!rule.handleEvent(Event(EventTypes::KeyStateKeyDown)));
}

void InputFilterTests::activateAndDeactivateActionsRunInOrder()
{
  int activated = 0;
  int deactivated = 0;

  InputFilter::Rule rule{new ToggleCondition()};
  rule.adoptAction(new CountingAction(&activated), true);
  rule.adoptAction(new CountingAction(&deactivated), false);

  QVERIFY(rule.handleEvent(Event(EventTypes::System)));
  QCOMPARE(activated, 1);
  QCOMPARE(deactivated, 0);

  QVERIFY(rule.handleEvent(Event(EventTypes::KeyStateKeyDown)));
  QCOMPARE(activated, 1);
  QCOMPARE(deactivated, 1);

  // an unrelated event matches neither list
  QVERIFY(!rule.handleEvent(Event(EventTypes::KeyStateKeyUp)));
  QCOMPARE(activated, 1);
  QCOMPARE(deactivated, 1);
}

void InputFilterTests::addFilterRuleRegistersRule()
{
  InputFilter filter(nullptr);
  QCOMPARE(filter.getNumRules(), 0u);

  filter.addFilterRule(InputFilter::Rule{new SystemOnlyCondition()});
  QCOMPARE(filter.getNumRules(), 1u);
}

QTEST_MAIN(InputFilterTests)
