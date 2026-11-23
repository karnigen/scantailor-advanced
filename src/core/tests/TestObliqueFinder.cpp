// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPL v3 license that can be found in the LICENSE file.

#include <BinaryImage.h>
#include <SkewFinder.h>
#include <filters/deskew/ObliqueFinder.h>

#include <catch2/catch_test_macros.hpp>

namespace Tests {
using namespace deskew;
using namespace imageproc;


TEST_CASE("ObliqueFinderTestSuite / build_sausage_mask_null_returns_null") {
  const BinaryImage empty;
  const BinaryImage result = buildSausageMask(empty);
  CHECK(result.isNull());
}

TEST_CASE("ObliqueFinderTestSuite / build_sausage_mask_preserves_size") {
  BinaryImage img(50, 30, WHITE);
  img.fill(QRect(10, 5, 20, 10), BLACK);
  const BinaryImage result = buildSausageMask(img);
  REQUIRE(!result.isNull());
  CHECK((result.width()) == (50));
  CHECK((result.height()) == (30));
}

TEST_CASE("ObliqueFinderTestSuite / find_oblique_null_mask_returns_nullopt") {
  const BinaryImage empty;
  SkewFinder finder;
  const auto result = findObliqueDegrees(empty, finder, 5.0);
  CHECK(!result.has_value());
}

}  // namespace Tests
