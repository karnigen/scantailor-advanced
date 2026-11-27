// SPDX-FileCopyrightText: 2026 ScanTailor Advanced contributors
// SPDX-License-Identifier: GPL-3.0-only

#include <EditableZoneSet.h>

#include <catch2/catch_test_macros.hpp>

#include <memory>


TEST_CASE("EditableZoneSetTestSuite / test_add_remove_preserves_order_and_uniqueness") {
  EditableZoneSet zones;
  const auto first = std::make_shared<EditableSpline>();
  const auto second = std::make_shared<EditableSpline>();

  zones.addZone(first);
  zones.addZone(second);
  zones.addZone(first);

  auto it = zones.begin();
  REQUIRE(it != zones.end());
  CHECK(it->spline() == first);
  ++it;
  REQUIRE(it != zones.end());
  CHECK(it->spline() == second);
  ++it;
  CHECK(it == zones.end());

  zones.removeZone(first);
  it = zones.begin();
  REQUIRE(it != zones.end());
  CHECK(it->spline() == second);
  ++it;
  CHECK(it == zones.end());
  CHECK(zones.propertiesFor(first) == nullptr);
  CHECK(zones.propertiesFor(second) != nullptr);
}
