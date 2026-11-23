// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include <DefaultParams.h>
#include <Dpi.h>
#include <ImageTransformation.h>
#include <filters/deskew/Dependencies.h>
#include <filters/deskew/Params.h>
#include <filters/deskew/Settings.h>
#include <filters/deskew/Utils.h>

#include <QDomDocument>
#include <QPolygonF>
#include <QRectF>
#include <QTransform>
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <memory>

namespace Tests {
using namespace deskew;


TEST_CASE("DeskewParamsTestSuite / dependencies_track_deskew_algorithm_and_xml") {
  QPolygonF outline;
  outline << QPointF(0, 0) << QPointF(100, 0) << QPointF(100, 100) << QPointF(0, 100);
  const Dependencies contentBased(outline, OrthogonalRotation(), true);
  const Dependencies topEdge(outline, OrthogonalRotation(), false);

  CHECK(contentBased.matches(contentBased));
  CHECK(!contentBased.matches(topEdge));

  QDomDocument doc;
  const QDomElement el = contentBased.toXml(doc, "dependencies");
  doc.appendChild(el);

  CHECK(el.attribute("algoContentBased") == "1");
  CHECK(contentBased.matches(Dependencies(doc.documentElement())));
}

TEST_CASE("DeskewParamsTestSuite / legacy_dependencies_are_stale_for_deskew_algorithm") {
  QPolygonF outline;
  outline << QPointF(0, 0) << QPointF(100, 0) << QPointF(100, 100) << QPointF(0, 100);
  const Dependencies legacy(outline, OrthogonalRotation());
  const Dependencies current(outline, OrthogonalRotation(), true);

  QDomDocument doc;
  const QDomElement el = legacy.toXml(doc, "dependencies");
  doc.appendChild(el);

  const Dependencies restoredLegacy(doc.documentElement());
  CHECK(!el.hasAttribute("algoContentBased"));
  CHECK(!current.matches(restoredLegacy));
}

TEST_CASE("DeskewParamsTestSuite / pending_recompute_is_persisted_as_stale_dependencies") {
  Settings settings;
  const PageId pageId(ImageId(QStringLiteral("pending-recompute.tif")));
  QPolygonF outline;
  outline << QPointF(0, 0) << QPointF(100, 0) << QPointF(100, 100) << QPointF(0, 100);
  const Dependencies currentDependencies(outline, OrthogonalRotation(), true);
  settings.setPageParams(pageId, Params(1.0, 0.0, currentDependencies, MODE_AUTO, MODE_DISABLED));
  settings.requestRecompute(pageId, true, false);

  const std::unique_ptr<Params> pendingParams(settings.getPageParams(pageId));
  REQUIRE(pendingParams);
  CHECK(!currentDependencies.matches(pendingParams->dependencies()));

  QDomDocument doc;
  const QDomElement el = pendingParams->toXml(doc, "params");
  doc.appendChild(el);
  const Params restored(doc.documentElement());
  CHECK(!currentDependencies.matches(restored.dependencies()));
}

TEST_CASE("DeskewParamsTestSuite / pending_oblique_recompute_is_persisted_as_stale_dependencies") {
  Settings settings;
  const PageId pageId(ImageId(QStringLiteral("pending-oblique-recompute.tif")));
  QPolygonF outline;
  outline << QPointF(0, 0) << QPointF(100, 0) << QPointF(100, 100) << QPointF(0, 100);
  const Dependencies currentDependencies(outline, OrthogonalRotation(), true);
  settings.setPageParams(pageId, Params(1.0, 0.0, currentDependencies, MODE_MANUAL, MODE_AUTO));
  settings.requestRecompute(pageId, false, true);

  const std::unique_ptr<Params> pendingParams(settings.getPageParams(pageId));
  REQUIRE(pendingParams);
  CHECK(!currentDependencies.matches(pendingParams->dependencies()));
}

TEST_CASE("DeskewParamsTestSuite / params_oblique_roundtrip_xml") {
  const double deskewDeg = 1.5;
  const double obliqueDeg = 2.25;
  const Dependencies deps;
  const Params original(deskewDeg, obliqueDeg, deps, MODE_MANUAL, MODE_MANUAL);

  QDomDocument doc;
  const QDomElement el = original.toXml(doc, "deskew-params");
  doc.appendChild(el);

  const Params restored(doc.documentElement());

  CHECK((restored.deskewAngle()) == Catch::Approx(deskewDeg).epsilon((1e-6) / 100.0));
  CHECK((restored.obliqueAngle()) == Catch::Approx(obliqueDeg).epsilon((1e-6) / 100.0));
  CHECK(restored.mode() == MODE_MANUAL);
  CHECK(restored.obliqueMode() == MODE_MANUAL);
}

TEST_CASE("DeskewParamsTestSuite / params_zero_oblique_roundtrip_xml") {
  const double deskewDeg = -0.5;
  const Dependencies deps;
  const Params original(deskewDeg, 0.0, deps, MODE_AUTO, MODE_AUTO);

  QDomDocument doc;
  const QDomElement el = original.toXml(doc, "deskew-params");
  doc.appendChild(el);

  const Params restored(doc.documentElement());

  CHECK((restored.deskewAngle()) == Catch::Approx(deskewDeg).epsilon((1e-6) / 100.0));
  CHECK(Catch::Approx(0.0).margin(1e-9) == (restored.obliqueAngle()));
  CHECK(restored.mode() == MODE_AUTO);
  CHECK(restored.obliqueMode() == MODE_AUTO);
}

TEST_CASE("DeskewParamsTestSuite / params_independent_oblique_mode_xml") {
  const Dependencies deps;
  const Params original(1.0, 2.0, deps, MODE_MANUAL, MODE_AUTO);

  QDomDocument doc;
  const QDomElement el = original.toXml(doc, "deskew-params");
  doc.appendChild(el);

  const Params restored(doc.documentElement());

  CHECK(restored.mode() == MODE_MANUAL);
  CHECK(restored.obliqueMode() == MODE_AUTO);
}

TEST_CASE("DeskewParamsTestSuite / params_oblique_mode_attribute_roundtrip_xml") {
  const Dependencies deps;
  const Params original(-1.25, 0.75, deps, MODE_AUTO, MODE_MANUAL);

  QDomDocument doc;
  const QDomElement el = original.toXml(doc, "deskew-params");
  doc.appendChild(el);

  CHECK(el.hasAttribute("oblique-mode"));
  CHECK(el.attribute("oblique-mode") == "manual");

  const Params restored(doc.documentElement());
  CHECK(restored.mode() == MODE_AUTO);
  CHECK(restored.obliqueMode() == MODE_MANUAL);
  CHECK((restored.deskewAngle()) == Catch::Approx(-1.25).epsilon((1e-6) / 100.0));
  CHECK((restored.obliqueAngle()) == Catch::Approx(0.75).epsilon((1e-6) / 100.0));
}

TEST_CASE("DeskewParamsTestSuite / params_disabled_oblique_mode_roundtrip_xml") {
  const Dependencies deps;
  const Params original(-0.5, 0.75, deps, MODE_AUTO, MODE_DISABLED);

  QDomDocument doc;
  const QDomElement el = original.toXml(doc, "deskew-params");
  doc.appendChild(el);

  CHECK(el.attribute("oblique-mode") == "manual");
  CHECK(el.attribute("oblique-disabled") == "1");
  CHECK(Catch::Approx(0.0).margin(1e-9) == (original.obliqueAngle()));
  const Params restored(doc.documentElement());
  CHECK(restored.mode() == MODE_AUTO);
  CHECK(restored.obliqueMode() == MODE_DISABLED);
  CHECK(!restored.autoOblique());
}

TEST_CASE("DeskewParamsTestSuite / params_legacy_auto_oblique_off_preserves_manual_angle") {
  QDomDocument doc;
  QDomElement el(doc.createElement("params"));
  el.setAttribute("mode", "auto");
  el.setAttribute("angle", "0");
  el.setAttribute("oblique", "0.75");
  el.setAttribute("autoOblique", "0");
  const Dependencies deps;
  el.appendChild(deps.toXml(doc, "dependencies"));
  doc.appendChild(el);

  const Params restored(doc.documentElement());
  CHECK(restored.obliqueMode() == MODE_MANUAL);
  CHECK((restored.obliqueAngle()) == Catch::Approx(0.75).epsilon((1e-6) / 100.0));
}

/** Selective Apply To… merge semantics (issue #117): deskew-only must not overwrite oblique. */
TEST_CASE("DeskewParamsTestSuite / apply_merge_deskew_only_preserves_oblique") {
  const Dependencies deps;
  const Params existing(1.0, 3.0, deps, MODE_MANUAL, MODE_MANUAL);
  const Params current(2.0, 4.0, deps, MODE_AUTO, MODE_AUTO);

  const Params merged(current.deskewAngle(), existing.obliqueAngle(), deps, current.mode(), existing.obliqueMode());

  CHECK((merged.deskewAngle()) == Catch::Approx(2.0).epsilon((1e-6) / 100.0));
  CHECK((merged.obliqueAngle()) == Catch::Approx(3.0).epsilon((1e-6) / 100.0));
  CHECK(merged.mode() == MODE_AUTO);
  CHECK(merged.obliqueMode() == MODE_MANUAL);
}

/** Selective Apply To… merge semantics (issue #117): oblique-only must not overwrite deskew. */
TEST_CASE("DeskewParamsTestSuite / apply_merge_oblique_only_preserves_deskew") {
  const Dependencies deps;
  const Params existing(1.0, 3.0, deps, MODE_MANUAL, MODE_MANUAL);
  const Params current(2.0, 4.0, deps, MODE_AUTO, MODE_AUTO);

  const Params merged(existing.deskewAngle(), current.obliqueAngle(), deps, existing.mode(), current.obliqueMode());

  CHECK((merged.deskewAngle()) == Catch::Approx(1.0).epsilon((1e-6) / 100.0));
  CHECK((merged.obliqueAngle()) == Catch::Approx(4.0).epsilon((1e-6) / 100.0));
  CHECK(merged.mode() == MODE_MANUAL);
  CHECK(merged.obliqueMode() == MODE_AUTO);
}

TEST_CASE("DeskewParamsTestSuite / params_auto_oblique_false_roundtrip_xml") {
  const Dependencies deps;
  const Params original(1.0, 0.25, deps, MODE_AUTO, MODE_MANUAL);

  QDomDocument doc;
  const QDomElement el = original.toXml(doc, "deskew-params");
  doc.appendChild(el);

  const Params restored(doc.documentElement());

  CHECK(restored.obliqueMode() == MODE_MANUAL);
  CHECK(!restored.autoOblique());
  CHECK((restored.obliqueAngle()) == Catch::Approx(0.25).epsilon((1e-6) / 100.0));
}

TEST_CASE("DeskewParamsTestSuite / params_missing_autoOblique_attribute_defaults_true") {
  QDomDocument doc;
  QDomElement el(doc.createElement("params"));
  el.setAttribute("mode", "auto");
  el.setAttribute("angle", "0");
  el.setAttribute("oblique", "0");
  const Dependencies deps;
  el.appendChild(deps.toXml(doc, "dependencies"));
  doc.appendChild(el);

  const Params restored(doc.documentElement());
  CHECK(restored.autoOblique());
}

TEST_CASE("DeskewParamsTestSuite / default_params_deskew_auto_oblique_off_by_default") {
  const DefaultParams::DeskewParams deskew;
  CHECK(!deskew.isAutoOblique());
}

TEST_CASE("DeskewParamsTestSuite / new_page_oblique_mode_follows_global_default") {
  DefaultParams::DeskewParams deskewParams;
  deskewParams.setAutoOblique(false);
  CHECK(Utils::buildDefaultParams(deskewParams).obliqueMode() == MODE_DISABLED);

  deskewParams.setAutoOblique(true);
  CHECK(Utils::buildDefaultParams(deskewParams).obliqueMode() == MODE_AUTO);
}


TEST_CASE("ImageTransformationObliqueTestSuite / post_oblique_applied_and_stored") {
  const QRectF rect(0, 0, 100, 100);
  const Dpi dpi(300, 300);
  ImageTransformation xform(rect, dpi);

  QPolygonF preCrop;
  preCrop << rect.topLeft() << rect.topRight() << rect.bottomRight() << rect.bottomLeft();
  xform.setPreCropArea(preCrop);

  xform.setPostOblique(0.0);
  const QTransform transformZeroOblique = xform.transform();
  const double obliqueDeg = 2.0;
  xform.setPostOblique(obliqueDeg);

  CHECK((xform.postOblique()) == Catch::Approx(obliqueDeg).epsilon((1e-6) / 100.0));
  CHECK(transformZeroOblique != xform.transform());
}

}  // namespace Tests
