// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#include <LinearFunction.h>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>


TEST_CASE("MathLinearFunctionTestSuite / test_constructor_num_vars") {
  LinearFunction f(3);
  CHECK((f.numVars()) == (3u));
  CHECK((f.a.size()) == (3u));
  CHECK(Catch::Approx(0.0).margin(1e-10) == (f.b));
}

TEST_CASE("MathLinearFunctionTestSuite / test_evaluate_linear") {
  LinearFunction f(2);
  f.a[0] = 1.0;
  f.a[1] = 2.0;
  f.b = 3.0;
  double x[] = {4.0, 5.0};
  const double val = f.evaluate(x);
  CHECK((val) == Catch::Approx(1.0 * 4.0 + 2.0 * 5.0 + 3.0).epsilon((1e-10) / 100.0));
  CHECK((val) == Catch::Approx(17.0).epsilon((1e-10) / 100.0));
}

TEST_CASE("MathLinearFunctionTestSuite / test_reset") {
  LinearFunction f(2);
  f.a[0] = 1.0;
  f.a[1] = 1.0;
  f.b = 1.0;
  f.reset();
  CHECK(Catch::Approx(0.0).margin(1e-10) == (f.a[0]));
  CHECK(Catch::Approx(0.0).margin(1e-10) == (f.a[1]));
  CHECK(Catch::Approx(0.0).margin(1e-10) == (f.b));
}

TEST_CASE("MathLinearFunctionTestSuite / test_operator_plus_equals") {
  LinearFunction f1(2);
  f1.a[0] = 1.0;
  f1.a[1] = 0.0;
  f1.b = 2.0;
  LinearFunction f2(2);
  f2.a[0] = 0.0;
  f2.a[1] = 3.0;
  f2.b = 1.0;
  f1 += f2;
  CHECK((f1.a[0]) == Catch::Approx(1.0).epsilon((1e-10) / 100.0));
  CHECK((f1.a[1]) == Catch::Approx(3.0).epsilon((1e-10) / 100.0));
  CHECK((f1.b) == Catch::Approx(3.0).epsilon((1e-10) / 100.0));
}

TEST_CASE("MathLinearFunctionTestSuite / test_operator_times_equals_scalar") {
  LinearFunction f(2);
  f.a[0] = 2.0;
  f.a[1] = 4.0;
  f.b = 6.0;
  f *= 0.5;
  CHECK((f.a[0]) == Catch::Approx(1.0).epsilon((1e-10) / 100.0));
  CHECK((f.a[1]) == Catch::Approx(2.0).epsilon((1e-10) / 100.0));
  CHECK((f.b) == Catch::Approx(3.0).epsilon((1e-10) / 100.0));
}

TEST_CASE("MathLinearFunctionTestSuite / test_swap") {
  LinearFunction f1(2);
  f1.a[0] = 1.0;
  f1.b = 2.0;
  LinearFunction f2(3);
  f2.a[0] = 5.0;
  f2.b = 6.0;
  swap(f1, f2);
  CHECK((f1.numVars()) == (3u));
  CHECK((f1.b) == Catch::Approx(6.0).epsilon((1e-10) / 100.0));
  CHECK((f2.numVars()) == (2u));
  CHECK((f2.b) == Catch::Approx(2.0).epsilon((1e-10) / 100.0));
}
