// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#include <LineIntersectionScalar.h>

#include <QLineF>
#include <QPointF>
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>


TEST_CASE("FoundationLineIntersectionScalarTestSuite / test_intersecting_lines") {
  const QLineF line1(0, 0, 10, 0);
  const QLineF line2(5, -5, 5, 5);
  double s1 = 0, s2 = 0;
  const bool ok = lineIntersectionScalar(line1, line2, s1, s2);
  REQUIRE(ok);
  CHECK((s1) == Catch::Approx(0.5).epsilon((0.01) / 100.0));
  CHECK((s2) == Catch::Approx(0.5).epsilon((0.01) / 100.0));
  const QPointF pt1 = line1.pointAt(s1);
  const QPointF pt2 = line2.pointAt(s2);
  CHECK((pt1.x()) == Catch::Approx(5.0).epsilon((0.01) / 100.0));
  CHECK((pt1.y()) == Catch::Approx(0.0).epsilon((0.01) / 100.0));
  CHECK((pt2.x()) == Catch::Approx(5.0).epsilon((0.01) / 100.0));
  CHECK((pt2.y()) == Catch::Approx(0.0).epsilon((0.01) / 100.0));
}

TEST_CASE("FoundationLineIntersectionScalarTestSuite / test_parallel_lines") {
  const QLineF line1(0, 0, 10, 0);
  const QLineF line2(0, 1, 10, 1);
  double s1 = 0, s2 = 0;
  const bool ok = lineIntersectionScalar(line1, line2, s1, s2);
  CHECK(!ok);
}

TEST_CASE("FoundationLineIntersectionScalarTestSuite / test_overlapping_lines") {
  const QLineF line1(0, 0, 10, 0);
  const QLineF line2(2, 0, 8, 0);
  double s1 = 0, s2 = 0;
  const bool ok = lineIntersectionScalar(line1, line2, s1, s2);
  CHECK(!ok);
}

TEST_CASE("FoundationLineIntersectionScalarTestSuite / test_single_param_overload") {
  const QLineF line1(0, 0, 10, 0);
  const QLineF line2(3, 1, 3, -1);
  double s1 = 0;
  const bool ok = lineIntersectionScalar(line1, line2, s1);
  REQUIRE(ok);
  CHECK((s1) == Catch::Approx(0.3).epsilon((0.01) / 100.0));
  CHECK((line1.pointAt(s1).x()) == Catch::Approx(3.0).epsilon((0.01) / 100.0));
}
