// Copyright (C) 2026  ScanTailor Advanced contributors
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "InteractionState.h"

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(InteractionStateTestSuite)

BOOST_AUTO_TEST_CASE(test_capture_moves_captor_from_proximity) {
  InteractionState state;
  InteractionState::Captor first;
  InteractionState::Captor second;

  state.updateProximity(first, Proximity::fromDist(1.0));
  BOOST_CHECK(state.proximityLeader(first));
  state.updateProximity(second, Proximity::fromDist(0.5));
  BOOST_CHECK(!first.is_linked());
  BOOST_CHECK(state.proximityLeader(second));

  state.capture(second);
  BOOST_CHECK(state.captured());
  BOOST_CHECK(state.capturedBy(second));
  BOOST_CHECK(!state.proximityLeader(second));
}

BOOST_AUTO_TEST_CASE(test_captor_copy_and_assignment_transfer_membership) {
  InteractionState state;
  InteractionState::Captor original;
  state.capture(original);

  InteractionState::Captor copied(original);
  BOOST_CHECK(!original.is_linked());
  BOOST_CHECK(state.capturedBy(copied));

  InteractionState::Captor replacement;
  state.capture(replacement);
  replacement = copied;
  BOOST_CHECK(!copied.is_linked());
  BOOST_CHECK(state.capturedBy(replacement));
}

BOOST_AUTO_TEST_CASE(test_nonadjacent_and_cross_state_membership_transfer) {
  InteractionState state;
  InteractionState::Captor first;
  InteractionState::Captor middle;
  InteractionState::Captor last;
  state.capture(first);
  state.capture(middle);
  state.capture(last);

  first = last;
  BOOST_CHECK(!last.is_linked());
  BOOST_CHECK(state.capturedBy(first));

  InteractionState otherState;
  InteractionState::Captor other;
  otherState.capture(other);
  first = other;
  BOOST_CHECK(!state.capturedBy(first));
  BOOST_CHECK(!other.is_linked());
  BOOST_CHECK(otherState.capturedBy(first));
}

BOOST_AUTO_TEST_CASE(test_captors_unlink_when_either_side_is_destroyed) {
  InteractionState::Captor outlivingCaptor;
  {
    InteractionState state;
    state.capture(outlivingCaptor);
    BOOST_CHECK(outlivingCaptor.is_linked());
  }
  BOOST_CHECK(!outlivingCaptor.is_linked());

  InteractionState state;
  {
    InteractionState::Captor temporaryCaptor;
    state.capture(temporaryCaptor);
  }
  BOOST_CHECK(!state.captured());
}

BOOST_AUTO_TEST_SUITE_END()
