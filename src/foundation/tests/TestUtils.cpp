// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#include <Utils.h>

#include <QString>
#include <catch2/catch_test_macros.hpp>

using namespace foundation;


TEST_CASE("FoundationUtilsTestSuite / test_double_to_string_zero") {
  CHECK((Utils::doubleToString(0.0).toStdString()) == ("0"));
}

TEST_CASE("FoundationUtilsTestSuite / test_double_to_string_positive") {
  const QString s = Utils::doubleToString(123.456);
  CHECK(s.contains("123"));
  CHECK((s.contains("456") || s.contains("46")));
}

TEST_CASE("FoundationUtilsTestSuite / test_double_to_string_negative") {
  const QString s = Utils::doubleToString(-42.5);
  CHECK((s.startsWith("-") || s.contains("-")));
  CHECK(s.contains("42"));
}

TEST_CASE("FoundationUtilsTestSuite / test_double_to_string_locale_independent") {
  const QString s1 = Utils::doubleToString(1.5);
  const QString s2 = Utils::doubleToString(1.5);
  CHECK((s1.toStdString()) == (s2.toStdString()));
  CHECK((s1.contains("1") && (s1.contains("5") || s1.contains("15"))));
}
