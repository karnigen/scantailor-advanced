// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include <Proximity.h>

#include <QLineF>
#include <QPointF>
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>


TEST_CASE("FoundationProximityTestSuite / test_default_constructor") {
  const Proximity p;
  CHECK(p.dist() > 1e10);
  CHECK(p.sqDist() > 1e20);
}

TEST_CASE("FoundationProximityTestSuite / test_two_points") {
  const QPointF a(0, 0);
  const QPointF b(3, 4);
  const Proximity p(a, b);
  CHECK((p.dist()) == Catch::Approx(5.0).epsilon((0.01) / 100.0));
  CHECK((p.sqDist()) == Catch::Approx(25.0).epsilon((0.01) / 100.0));
}

TEST_CASE("FoundationProximityTestSuite / test_same_point") {
  const QPointF a(1, 2);
  const Proximity p(a, a);
  CHECK(Catch::Approx(0.0).margin(1e-10) == (p.dist()));
  CHECK(Catch::Approx(0.0).margin(1e-20) == (p.sqDist()));
}

TEST_CASE("FoundationProximityTestSuite / test_from_dist") {
  const Proximity p = Proximity::fromDist(10.0);
  CHECK((p.dist()) == Catch::Approx(10.0).epsilon((0.01) / 100.0));
  CHECK((p.sqDist()) == Catch::Approx(100.0).epsilon((0.01) / 100.0));
}

TEST_CASE("FoundationProximityTestSuite / test_from_sq_dist") {
  const Proximity p = Proximity::fromSqDist(49.0);
  CHECK((p.sqDist()) == Catch::Approx(49.0).epsilon((0.01) / 100.0));
  CHECK((p.dist()) == Catch::Approx(7.0).epsilon((0.01) / 100.0));
}

TEST_CASE("FoundationProximityTestSuite / test_comparison_operators") {
  const Proximity a = Proximity::fromSqDist(1.0);
  const Proximity b = Proximity::fromSqDist(4.0);
  CHECK(a < b);
  CHECK(a <= b);
  CHECK(b > a);
  CHECK(b >= a);
  CHECK(a != b);
  CHECK(!(a == b));
  CHECK(Proximity::fromSqDist(2.0) == Proximity::fromSqDist(2.0));
}

TEST_CASE("FoundationProximityTestSuite / test_point_and_line_segment_perpendicular") {
  const QPointF pt(5, 0);
  const QLineF segment(0, 0, 10, 0);
  QPointF pointOnSegment;
  const Proximity p = Proximity::pointAndLineSegment(pt, segment, &pointOnSegment);
  CHECK((p.dist()) == Catch::Approx(0.0).epsilon((0.01) / 100.0));
  CHECK((pointOnSegment.x()) == Catch::Approx(5.0).epsilon((0.01) / 100.0));
  CHECK((pointOnSegment.y()) == Catch::Approx(0.0).epsilon((0.01) / 100.0));
}

TEST_CASE("FoundationProximityTestSuite / test_point_and_line_segment_endpoint_closest") {
  const QPointF pt(-1, 0);
  const QLineF segment(0, 0, 10, 0);
  QPointF pointOnSegment;
  const Proximity prx = Proximity::pointAndLineSegment(pt, segment, &pointOnSegment);
  CHECK((prx.dist()) == Catch::Approx(1.0).epsilon((0.01) / 100.0));
  CHECK((pointOnSegment.x()) == Catch::Approx(0.0).epsilon((0.01) / 100.0));
  CHECK((pointOnSegment.y()) == Catch::Approx(0.0).epsilon((0.01) / 100.0));
}

TEST_CASE("FoundationProximityTestSuite / test_point_and_zero_length_segment") {
  const QPointF pt(1, 1);
  const QLineF segment(0, 0, 0, 0);
  const Proximity p = Proximity::pointAndLineSegment(pt, segment);
  CHECK((p.dist()) == Catch::Approx(std::sqrt(2.0)).epsilon((0.01) / 100.0));
}
