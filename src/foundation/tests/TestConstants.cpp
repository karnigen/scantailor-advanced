// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include <Constants.h>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <cmath>


TEST_CASE("FoundationConstantsTestSuite / test_pi") {
  CHECK((constants::PI) == Catch::Approx(3.14159265358979323846).epsilon((0.0001) / 100.0));
}

TEST_CASE("FoundationConstantsTestSuite / test_sqrt_2") {
  CHECK((constants::SQRT_2) == Catch::Approx(std::sqrt(2.0)).epsilon((0.0001) / 100.0));
}

TEST_CASE("FoundationConstantsTestSuite / test_deg2rad_rad2deg") {
  CHECK((constants::DEG2RAD * 180.0) == Catch::Approx(constants::PI).epsilon((0.0001) / 100.0));
  CHECK((constants::RAD2DEG * constants::PI) == Catch::Approx(180.0).epsilon((0.0001) / 100.0));
}

TEST_CASE("FoundationConstantsTestSuite / test_inch_mm_conversion") {
  CHECK((constants::INCH2MM) == Catch::Approx(25.4).epsilon((0.0001) / 100.0));
  CHECK((constants::MM2INCH * 25.4) == Catch::Approx(1.0).epsilon((0.0001) / 100.0));
}

TEST_CASE("FoundationConstantsTestSuite / test_dpi_dpm_conversion") {
  CHECK((constants::DPM2DPI) == Catch::Approx(0.0254).epsilon((0.0001) / 100.0));
  CHECK((constants::DPI2DPM * constants::DPM2DPI) == Catch::Approx(1.0).epsilon((0.0001) / 100.0));
}
