// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#include <Units.h>

#include <QString>
#include <catch2/catch_test_macros.hpp>


TEST_CASE("CoreUnitsTestSuite / test_units_to_string") {
  CHECK(unitsToString(PIXELS) == "px");
  CHECK(unitsToString(MILLIMETRES) == "mm");
  CHECK(unitsToString(CENTIMETRES) == "cm");
  CHECK(unitsToString(INCHES) == "in");
}

TEST_CASE("CoreUnitsTestSuite / test_units_from_string") {
  CHECK(unitsFromString("px") == PIXELS);
  CHECK(unitsFromString("mm") == MILLIMETRES);
  CHECK(unitsFromString("cm") == CENTIMETRES);
  CHECK(unitsFromString("in") == INCHES);
}

TEST_CASE("CoreUnitsTestSuite / test_units_from_string_defaults_to_mm") {
  CHECK(unitsFromString("") == MILLIMETRES);
  CHECK(unitsFromString("unknown") == MILLIMETRES);
}

TEST_CASE("CoreUnitsTestSuite / test_units_to_localized_string") {
  QString px = unitsToLocalizedString(PIXELS);
  QString mm = unitsToLocalizedString(MILLIMETRES);
  QString cm = unitsToLocalizedString(CENTIMETRES);
  QString in = unitsToLocalizedString(INCHES);
  CHECK(!px.isEmpty());
  CHECK(!mm.isEmpty());
  CHECK(!cm.isEmpty());
  CHECK(!in.isEmpty());
}
