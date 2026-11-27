// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#include <SidesOfLine.h>

#include <QLineF>
#include <QPointF>
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>


TEST_CASE("MathSidesOfLineTestSuite / test_points_on_same_side_positive") {
  const QLineF line(0, 0, 10, 0);
  const QPointF p1(5, 1);
  const QPointF p2(5, 2);
  const qreal result = sidesOfLine(line, p1, p2);
  CHECK(result > 0);
}

TEST_CASE("MathSidesOfLineTestSuite / test_points_on_opposite_sides") {
  const QLineF line(0, 0, 10, 0);
  const QPointF p1(5, 1);
  const QPointF p2(5, -1);
  const qreal result = sidesOfLine(line, p1, p2);
  CHECK(result < 0);
}

TEST_CASE("MathSidesOfLineTestSuite / test_point_on_line_returns_zero") {
  const QLineF line(0, 0, 10, 0);
  const QPointF p1(5, 0);
  const QPointF p2(5, 1);
  const qreal result = sidesOfLine(line, p1, p2);
  CHECK(Catch::Approx(0.0).margin(1e-6) == (result));
}

TEST_CASE("MathSidesOfLineTestSuite / test_both_points_on_line_returns_zero") {
  const QLineF line(0, 0, 10, 0);
  const QPointF p1(2, 0);
  const QPointF p2(7, 0);
  const qreal result = sidesOfLine(line, p1, p2);
  CHECK(Catch::Approx(0.0).margin(1e-6) == (result));
}

TEST_CASE("MathSidesOfLineTestSuite / test_diagonal_line_same_side") {
  const QLineF line(0, 0, 10, 10);
  const QPointF p1(0, 1);
  const QPointF p2(1, 2);
  const qreal result = sidesOfLine(line, p1, p2);
  CHECK(result > 0);
}
