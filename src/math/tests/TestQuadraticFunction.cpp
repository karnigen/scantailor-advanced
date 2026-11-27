// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#include <QuadraticFunction.h>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>


TEST_CASE("MathQuadraticFunctionTestSuite / test_constructor") {
  QuadraticFunction f(2);
  CHECK((f.numVars()) == (2u));
  CHECK(Catch::Approx(0.0).margin(1e-10) == (f.c));
  CHECK((f.b.size()) == (2u));
  CHECK((f.A.rows()) == (2u));
  CHECK((f.A.cols()) == (2u));
}

TEST_CASE("MathQuadraticFunctionTestSuite / test_evaluate_quadratic") {
  QuadraticFunction f(2);
  f.A(0, 0) = 1.0;
  f.A(1, 1) = 1.0;
  f.b[0] = 1.0;
  f.b[1] = 1.0;
  f.c = 0.0;
  double x[] = {2.0, 3.0};
  const double val = f.evaluate(x);
  CHECK((val) == Catch::Approx(2 * 2 + 3 * 3 + 2 + 3).epsilon((1e-10) / 100.0));
  CHECK((val) == Catch::Approx(4.0 + 9.0 + 5.0).epsilon((1e-10) / 100.0));
}

TEST_CASE("MathQuadraticFunctionTestSuite / test_reset") {
  QuadraticFunction f(2);
  f.A(0, 0) = 1.0;
  f.b[0] = 1.0;
  f.c = 1.0;
  f.reset();
  CHECK(Catch::Approx(0.0).margin(1e-10) == (f.A(0, 0)));
  CHECK(Catch::Approx(0.0).margin(1e-10) == (f.b[0]));
  CHECK(Catch::Approx(0.0).margin(1e-10) == (f.c));
}

TEST_CASE("MathQuadraticFunctionTestSuite / test_operator_plus_equals") {
  QuadraticFunction f1(2);
  f1.A(0, 0) = 1.0;
  f1.b[0] = 1.0;
  f1.c = 1.0;
  QuadraticFunction f2(2);
  f2.A(1, 1) = 1.0;
  f2.b[1] = 1.0;
  f2.c = 2.0;
  f1 += f2;
  CHECK((f1.A(0, 0)) == Catch::Approx(1.0).epsilon((1e-10) / 100.0));
  CHECK((f1.A(1, 1)) == Catch::Approx(1.0).epsilon((1e-10) / 100.0));
  CHECK((f1.b[0]) == Catch::Approx(1.0).epsilon((1e-10) / 100.0));
  CHECK((f1.b[1]) == Catch::Approx(1.0).epsilon((1e-10) / 100.0));
  CHECK((f1.c) == Catch::Approx(3.0).epsilon((1e-10) / 100.0));
}

TEST_CASE("MathQuadraticFunctionTestSuite / test_operator_times_equals_scalar") {
  QuadraticFunction f(2);
  f.A(0, 0) = 2.0;
  f.b[0] = 4.0;
  f.c = 6.0;
  f *= 0.5;
  CHECK((f.A(0, 0)) == Catch::Approx(1.0).epsilon((1e-10) / 100.0));
  CHECK((f.b[0]) == Catch::Approx(2.0).epsilon((1e-10) / 100.0));
  CHECK((f.c) == Catch::Approx(3.0).epsilon((1e-10) / 100.0));
}

TEST_CASE("MathQuadraticFunctionTestSuite / test_gradient") {
  QuadraticFunction f(2);
  f.A(0, 0) = 2.0;
  f.A(0, 1) = 1.0;
  f.A(1, 0) = 1.0;
  f.A(1, 1) = 2.0;
  f.b[0] = 1.0;
  f.b[1] = 2.0;
  f.c = 0.0;
  const auto grad = f.gradient();
  CHECK((grad.A.rows()) == (2u));
  CHECK((grad.A.cols()) == (2u));
  CHECK((grad.b[0]) == Catch::Approx(1.0).epsilon((1e-10) / 100.0));
  CHECK((grad.b[1]) == Catch::Approx(2.0).epsilon((1e-10) / 100.0));
}

TEST_CASE("MathQuadraticFunctionTestSuite / test_recalc_for_translated_arguments") {
  QuadraticFunction f(2);
  f.A(0, 0) = 1.0;
  f.b[0] = 0.0;
  f.c = 0.0;
  double trans[] = {1.0, 0.0};
  f.recalcForTranslatedArguments(trans);
  CHECK((f.b[0]) == Catch::Approx(2.0).epsilon((1e-10) / 100.0));
  CHECK((f.c) == Catch::Approx(1.0).epsilon((1e-10) / 100.0));
}
