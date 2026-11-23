// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include <filters/select_content/Dependencies.h>

#include <QPolygonF>
#include <catch2/catch_test_macros.hpp>

namespace Tests {
using namespace select_content;


TEST_CASE("SelectContentApplyTestSuite / apply_content_box_auto_forces_recompute") {
  QPolygonF pageOutline;
  pageOutline << QPointF(0, 0) << QPointF(100, 0) << QPointF(100, 150) << QPointF(0, 150);

  Dependencies currentDeps(pageOutline, MODE_AUTO, MODE_DISABLED, false);

  Dependencies storedDepsWhenApplyingAuto(QPolygonF(), MODE_DISABLED, MODE_DISABLED, false);

  bool needUpdateContentBox = false;
  bool needUpdatePageBox = false;
  currentDeps.compatibleWith(storedDepsWhenApplyingAuto, &needUpdateContentBox, &needUpdatePageBox);

  {
    INFO(
        "When applying 'Content Box: Auto' we store deps with MODE_DISABLED so that "
        "Task::process recomputes content box for each target page.");
    REQUIRE(needUpdateContentBox);
  }
  CHECK(needUpdatePageBox);
}

TEST_CASE("SelectContentApplyTestSuite / same_mode_auto_no_content_box_update") {
  QPolygonF pageOutline;
  pageOutline << QPointF(0, 0) << QPointF(100, 0) << QPointF(100, 150) << QPointF(0, 150);

  Dependencies deps1(pageOutline, MODE_AUTO, MODE_DISABLED, false);
  Dependencies deps2(pageOutline, MODE_AUTO, MODE_DISABLED, false);

  bool needUpdateContentBox = false;
  bool needUpdatePageBox = false;
  deps1.compatibleWith(deps2, &needUpdateContentBox, &needUpdatePageBox);

  CHECK(!needUpdateContentBox);
  CHECK(!needUpdatePageBox);
}

TEST_CASE("SelectContentApplyTestSuite / different_outlines_force_content_box_update") {
  QPolygonF outlineA;
  outlineA << QPointF(0, 0) << QPointF(100, 0) << QPointF(100, 150) << QPointF(0, 150);
  QPolygonF outlineB;
  outlineB << QPointF(0, 0) << QPointF(120, 0) << QPointF(120, 150) << QPointF(0, 150);

  Dependencies currentDeps(outlineA, MODE_AUTO, MODE_DISABLED, false);
  Dependencies storedDeps(outlineB, MODE_AUTO, MODE_DISABLED, false);

  bool needUpdateContentBox = false;
  bool needUpdatePageBox = false;
  currentDeps.compatibleWith(storedDeps, &needUpdateContentBox, &needUpdatePageBox);

  {
    INFO("When outlines differ, needUpdateContentBox must be true (PR #108 review).");
    REQUIRE(needUpdateContentBox);
  }
  CHECK(needUpdatePageBox);
}

}  // namespace Tests
