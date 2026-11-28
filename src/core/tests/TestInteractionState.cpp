// SPDX-FileCopyrightText: 2026 project contributors
// SPDX-License-Identifier: GPL-3.0-only

#include "InteractionState.h"

#include <catch2/catch_test_macros.hpp>


TEST_CASE("InteractionStateTestSuite / test_capture_moves_captor_from_proximity") {
  InteractionState state;
  InteractionState::Captor first;
  InteractionState::Captor second;

  state.updateProximity(first, Proximity::fromDist(1.0));
  CHECK(state.proximityLeader(first));
  state.updateProximity(second, Proximity::fromDist(0.5));
  CHECK(!first.is_linked());
  CHECK(state.proximityLeader(second));

  state.capture(second);
  CHECK(state.captured());
  CHECK(state.capturedBy(second));
  CHECK(!state.proximityLeader(second));
}

TEST_CASE("InteractionStateTestSuite / test_captor_copy_and_assignment_transfer_membership") {
  InteractionState state;
  InteractionState::Captor original;
  state.capture(original);

  InteractionState::Captor copied(original);
  CHECK(!original.is_linked());
  CHECK(state.capturedBy(copied));

  InteractionState::Captor replacement;
  state.capture(replacement);
  replacement = copied;
  CHECK(!copied.is_linked());
  CHECK(state.capturedBy(replacement));
}

TEST_CASE("InteractionStateTestSuite / test_nonadjacent_and_cross_state_membership_transfer") {
  InteractionState state;
  InteractionState::Captor first;
  InteractionState::Captor middle;
  InteractionState::Captor last;
  state.capture(first);
  state.capture(middle);
  state.capture(last);

  first = last;
  CHECK(!last.is_linked());
  CHECK(state.capturedBy(first));

  InteractionState otherState;
  InteractionState::Captor other;
  otherState.capture(other);
  first = other;
  CHECK(!state.capturedBy(first));
  CHECK(!other.is_linked());
  CHECK(otherState.capturedBy(first));
}

TEST_CASE("InteractionStateTestSuite / test_captors_unlink_when_either_side_is_destroyed") {
  InteractionState::Captor outlivingCaptor;
  {
    InteractionState state;
    state.capture(outlivingCaptor);
    CHECK(outlivingCaptor.is_linked());
  }
  CHECK(!outlivingCaptor.is_linked());

  InteractionState state;
  {
    InteractionState::Captor temporaryCaptor;
    state.capture(temporaryCaptor);
  }
  CHECK(!state.captured());
}
