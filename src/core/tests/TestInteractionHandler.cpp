// Copyright (C) 2026  ScanTailor Advanced contributors
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "InteractionHandler.h"
#include "InteractionState.h"

#include <QKeyEvent>
#include <boost/test/unit_test.hpp>

#include <vector>

namespace {
class RecordingHandler : public InteractionHandler {
 public:
  RecordingHandler(std::vector<int>& calls, int id) : m_calls(calls), m_id(id) {}

 protected:
  void onKeyPressEvent(QKeyEvent*, InteractionState&) override { m_calls.push_back(m_id); }

 private:
  std::vector<int>& m_calls;
  int m_id;
};
}  // namespace

BOOST_AUTO_TEST_SUITE(InteractionHandlerTestSuite)

BOOST_AUTO_TEST_CASE(test_ordering_and_unlinking) {
  std::vector<int> calls;
  InteractionHandler root;
  auto* first = new RecordingHandler(calls, 1);
  auto* second = new RecordingHandler(calls, 2);
  auto* preceeder = new RecordingHandler(calls, 0);
  auto* follower = new RecordingHandler(calls, 3);
  root.makeLastFollower(*first);
  root.makeLastFollower(*second);
  first->makePeerPreceeder(*preceeder);
  first->makePeerFollower(*follower);

  InteractionState interaction;
  QKeyEvent event(QEvent::KeyPress, Qt::Key_A, Qt::NoModifier);
  event.ignore();
  root.keyPressEvent(&event, interaction);
  BOOST_CHECK(calls == (std::vector<int>{0, 1, 3, 2}));

  calls.clear();
  first->unlink();
  BOOST_CHECK(!first->is_linked());
  delete first;

  {
    RecordingHandler transient(calls, 3);
    root.makeLastFollower(transient);
    BOOST_CHECK(transient.is_linked());
  }

  calls.clear();
  QKeyEvent secondEvent(QEvent::KeyPress, Qt::Key_A, Qt::NoModifier);
  secondEvent.ignore();
  root.keyPressEvent(&secondEvent, interaction);
  BOOST_CHECK(calls == (std::vector<int>{0, 3, 2}));
}

BOOST_AUTO_TEST_SUITE_END()
