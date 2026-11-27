// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#include <RastLineFinder.h>

#include <QLineF>
#include <QPointF>
#include <catch2/catch_test_macros.hpp>
#include <set>
#include <vector>

namespace imageproc {
namespace tests {

static bool matchSupportPoints(const std::vector<unsigned>& idxs1, const std::set<unsigned>& idxs2) {
  return std::set<unsigned>(idxs1.begin(), idxs1.end()) == idxs2;
}

TEST_CASE("RastLineFinderTestSuite / test1") {
  // 4- and 3-point lines with min_support_points == 3
  // --------------------------------------------------
  // x     x
  // x x
  // x x
  // x
  // --------------------------------------------------
  std::vector<QPointF> pts;
  pts.emplace_back(-100, -100);
  pts.emplace_back(0, 0);
  pts.emplace_back(100, 100);
  pts.emplace_back(200, 200);
  pts.emplace_back(0, 100);
  pts.emplace_back(100, 0);
  pts.emplace_back(-100, 200);

  std::set<unsigned> line1Idxs;
  line1Idxs.insert(0);
  line1Idxs.insert(1);
  line1Idxs.insert(2);
  line1Idxs.insert(3);

  std::set<unsigned> line2Idxs;
  line2Idxs.insert(4);
  line2Idxs.insert(5);
  line2Idxs.insert(6);

  RastLineFinderParams params;
  params.setMinSupportPoints(3);
  RastLineFinder finder(pts, params);

  std::vector<unsigned> supportIdxs;

  // line 1
  REQUIRE(!finder.findNext(&supportIdxs).isNull());
  REQUIRE(matchSupportPoints(supportIdxs, line1Idxs));

  // line2
  REQUIRE(!finder.findNext(&supportIdxs).isNull());
  REQUIRE(matchSupportPoints(supportIdxs, line2Idxs));

  // no more lines
  REQUIRE(finder.findNext().isNull());
}

}  // namespace tests
}  // namespace imageproc
