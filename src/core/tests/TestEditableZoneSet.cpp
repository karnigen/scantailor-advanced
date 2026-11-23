// Copyright (C) 2026  ScanTailor Advanced contributors
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include <EditableZoneSet.h>

#include <boost/test/unit_test.hpp>

#include <memory>

BOOST_AUTO_TEST_SUITE(EditableZoneSetTestSuite)

BOOST_AUTO_TEST_CASE(test_add_remove_preserves_order_and_uniqueness) {
  EditableZoneSet zones;
  const auto first = std::make_shared<EditableSpline>();
  const auto second = std::make_shared<EditableSpline>();

  zones.addZone(first);
  zones.addZone(second);
  zones.addZone(first);

  auto it = zones.begin();
  BOOST_REQUIRE(it != zones.end());
  BOOST_CHECK(it->spline() == first);
  ++it;
  BOOST_REQUIRE(it != zones.end());
  BOOST_CHECK(it->spline() == second);
  ++it;
  BOOST_CHECK(it == zones.end());

  zones.removeZone(first);
  it = zones.begin();
  BOOST_REQUIRE(it != zones.end());
  BOOST_CHECK(it->spline() == second);
  ++it;
  BOOST_CHECK(it == zones.end());
  BOOST_CHECK(zones.propertiesFor(first) == nullptr);
  BOOST_CHECK(zones.propertiesFor(second) != nullptr);
}

BOOST_AUTO_TEST_SUITE_END()
