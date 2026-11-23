// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include <LineBoundedByRect.h>

#include <QLineF>
#include <QRectF>
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>


TEST_CASE("MathLineBoundedByRectTestSuite / test_line_through_rect_center") {
  const QRectF rect(0, 0, 100, 100);
  QLineF line(-10, 50, 110, 50);
  const bool ok = lineBoundedByRect(line, rect);
  REQUIRE(ok);
  CHECK(Catch::Approx(0.0).margin(1e-9) == (line.p1().x()));
  CHECK((line.p1().y()) == Catch::Approx(50.0).epsilon((0.01) / 100.0));
  CHECK((line.p2().x()) == Catch::Approx(100.0).epsilon((0.01) / 100.0));
  CHECK((line.p2().y()) == Catch::Approx(50.0).epsilon((0.01) / 100.0));
}

TEST_CASE("MathLineBoundedByRectTestSuite / test_line_through_rect_diagonal") {
  const QRectF rect(0, 0, 100, 100);
  QLineF line(-5, -5, 105, 105);
  const bool ok = lineBoundedByRect(line, rect);
  REQUIRE(ok);
  CHECK(Catch::Approx(0.0).margin(1e-10) == (line.p1().x()));
  CHECK(Catch::Approx(0.0).margin(1e-10) == (line.p1().y()));
  CHECK((line.p2().x()) == Catch::Approx(100.0).epsilon((0.01) / 100.0));
  CHECK((line.p2().y()) == Catch::Approx(100.0).epsilon((0.01) / 100.0));
}

TEST_CASE("MathLineBoundedByRectTestSuite / test_line_outside_rect_no_intersection") {
  const QRectF rect(10, 10, 80, 80);
  QLineF line(5, 0, 5, 100);
  const bool ok = lineBoundedByRect(line, rect);
  CHECK(!ok);
}

TEST_CASE("MathLineBoundedByRectTestSuite / test_vertical_line_through_rect") {
  const QRectF rect(10, 10, 80, 80);
  QLineF line(50, 0, 50, 200);
  const bool ok = lineBoundedByRect(line, rect);
  REQUIRE(ok);
  CHECK((line.p1().x()) == Catch::Approx(50.0).epsilon((0.01) / 100.0));
  CHECK((line.p1().y()) == Catch::Approx(10.0).epsilon((0.01) / 100.0));
  CHECK((line.p2().x()) == Catch::Approx(50.0).epsilon((0.01) / 100.0));
  CHECK((line.p2().y()) == Catch::Approx(90.0).epsilon((0.01) / 100.0));
}
