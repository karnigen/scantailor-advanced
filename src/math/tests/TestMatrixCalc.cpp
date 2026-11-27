// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#include <MatrixCalc.h>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

namespace imageproc {
namespace tests {

TEST_CASE("MatrixCalcSuite / test1") {
  static const double A[] = {1, 1, 1, 2, 4, -3, 3, 6, -5};

  static const double B[] = {9, 1, 0};

  static const double control[] = {7, -1, 3};

  double x[3];

  MatrixCalc<double> mc;
  mc(A, 3, 3).trans().solve(mc(B, 3, 1)).write(x);

  for (int i = 0; i < 3; ++i) {
    REQUIRE((x[i]) == Catch::Approx(control[i]).epsilon((1e-6) / 100.0));
  }
}

TEST_CASE("MatrixCalcSuite / test2") {
  static const double A[] = {1, 1, 1, 2, 4, -3, 3, 6, -5, 3, 5, -2, 5, 10, -8};

  double B[] = {9, 1, 0, 10, 1};

  static const double control[] = {7, -1, 3};

  double x[3];

  MatrixCalc<double> mc;
  mc(A, 3, 5).trans().solve(mc(B, 5, 1)).write(x);

  for (int i = 0; i < 3; ++i) {
    REQUIRE((x[i]) == Catch::Approx(control[i]).epsilon((1e-6) / 100.0));
  }

  // Now make the system inconsistent.
  B[4] += 1.0;
  CHECK_THROWS_AS((mc(A, 3, 5).trans().solve(mc(B, 5, 1))), std::runtime_error);
}

TEST_CASE("MatrixCalcSuite / test3") {
  static const double A[] = {1, 3, 1, 1, 1, 2, 2, 3, 4};

  static const double control[] = {2, 9, -5, 0, -2, 1, -1, -3, 2};

  double inv[9];

  MatrixCalc<double> mc;
  mc(A, 3, 3).trans().inv().transWrite(inv);

  for (int i = 0; i < 9; ++i) {
    REQUIRE((inv[i]) == Catch::Approx(control[i]).epsilon((1e-6) / 100.0));
  }
}

TEST_CASE("MatrixCalcSuite / test4") {
  static const double A[] = {4, 1, 9, 6, 2, 8, 7, 3, 5, 11, 10, 12};

  static const double B[] = {2, 9, 5, 12, 8, 10};

  static const double control[] = {85, 138, 86, 158, 69, 149, 168, 339};

  double mul[8];

  MatrixCalc<double> mc;
  (mc(A, 3, 4).trans() * (mc(B, 2, 3).trans())).transWrite(mul);

  for (int i = 0; i < 8; ++i) {
    REQUIRE((mul[i]) == Catch::Approx(control[i]).epsilon((1e-6) / 100.0));
  }
}

}  // namespace tests
}  // namespace imageproc