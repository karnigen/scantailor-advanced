// Copyright (C) 2026  ScanTailor Advanced contributors
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "../filters/page_layout/Alignment.h"
#include "../filters/page_layout/OrderByHeightProvider.h"
#include "../filters/page_layout/OrderByWidthProvider.h"
#include "../filters/page_layout/Params.h"
#include "../filters/page_layout/Settings.h"
#include "../filters/page_layout/Utils.h"

#include <Dpi.h>
#include <ImageId.h>
#include <ImageTransformation.h>
#include <PageId.h>

#include <QPointF>
#include <QPolygonF>
#include <QRectF>
#include <QSizeF>
#include <algorithm>
#include <catch2/catch_test_macros.hpp>
#include <memory>
#include <vector>


TEST_CASE("PageLayoutSettingsTestSuite / test_aggregate_dimensions_track_independent_maxima") {
  page_layout::Settings settings;
  const PageId first(ImageId("/first.tif", 0), PageId::SINGLE_PAGE);
  const PageId second(ImageId("/second.tif", 0), PageId::SINGLE_PAGE);

  settings.setContentSizeMM(first, QSizeF(100.0, 50.0));
  settings.setContentSizeMM(second, QSizeF(200.0, 20.0));
  CHECK(settings.getAggregateHardSizeMM() == QSizeF(200.0, 50.0));

  settings.setContentSizeMM(second, QSizeF(50.0, 100.0));
  CHECK(settings.getAggregateHardSizeMM() == QSizeF(100.0, 100.0));

  settings.setHardMarginsMM(first, Margins(1000.0, 1000.0, 1000.0, 1000.0));
  CHECK(settings.getAggregateHardSizeMM() == QSizeF(100.0, 100.0));

  page_layout::Alignment unaligned;
  unaligned.setNull(true);
  settings.setPageAlignment(second, unaligned);
  CHECK(settings.getAggregateHardSizeMM() == QSizeF(100.0, 50.0));

  settings.setPageAlignment(first, unaligned);
  CHECK(settings.getAggregateHardSizeMM() == QSizeF(0.0, 0.0));
}

TEST_CASE("PageLayoutSettingsTestSuite / test_equal_dimensions_keep_both_ordered_entries") {
  page_layout::Settings settings;
  const PageId first(ImageId("/first.tif", 0), PageId::SINGLE_PAGE);
  const PageId second(ImageId("/second.tif", 0), PageId::SINGLE_PAGE);
  settings.setContentSizeMM(first, QSizeF(100.0, 50.0));
  settings.setContentSizeMM(second, QSizeF(100.0, 50.0));

  page_layout::Alignment unaligned;
  unaligned.setNull(true);
  settings.setPageAlignment(first, unaligned);
  CHECK(settings.getAggregateHardSizeMM() == QSizeF(100.0, 50.0));
}

TEST_CASE("PageLayoutSettingsTestSuite / active_page_boundary_contributes_to_common_size") {
  page_layout::Settings settings;
  const PageId pageId(ImageId("/page-boundary.tif", 0), PageId::SINGLE_PAGE);

  const page_layout::Params params(
      settings.updateContentSizeAndGetParams(pageId, QRectF(0.0, 0.0, 800.0, 600.0), QRectF(100.0, 100.0, 600.0, 400.0),
                                             QSizeF(100.0, 50.0), true, QSizeF(200.0, 150.0)));

  CHECK(params.isPageBoundaryEnabled());
  CHECK(params.pageBoundarySizeMM() == QSizeF(200.0, 150.0));
  CHECK(settings.getAggregateHardSizeMM() == QSizeF(200.0, 150.0));

  settings.setHardMarginsMM(pageId, Margins(1000.0, 1000.0, 1000.0, 1000.0));
  CHECK(settings.getAggregateHardSizeMM() == QSizeF(200.0, 150.0));
}

TEST_CASE("PageLayoutSettingsTestSuite / content larger than active page boundary contributes to common size") {
  page_layout::Settings settings;
  const PageId pageId(ImageId("/large-content.tif", 0), PageId::SINGLE_PAGE);

  settings.updateContentSizeAndGetParams(pageId, QRectF(0.0, 0.0, 800.0, 600.0), QRectF(50.0, 50.0, 700.0, 500.0),
                                         QSizeF(120.0, 60.0), true, QSizeF(80.0, 40.0));
  CHECK(settings.getAggregateHardSizeMM() == QSizeF(120.0, 60.0));

  settings.setHardMarginsMM(pageId, Margins(1000.0, 1000.0, 1000.0, 1000.0));
  CHECK(settings.getAggregateHardSizeMM() == QSizeF(120.0, 60.0));
}

TEST_CASE("PageLayoutSettingsTestSuite / hard margins are constrained when Page Box supplies common size") {
  const QPolygonF contentRectMm{QPointF(10.0, 10.0), QPointF(90.0, 10.0), QPointF(90.0, 190.0), QPointF(10.0, 190.0)};
  const Margins requestedMargins(30.0, 30.0, 30.0, 30.0);

  const Margins constrainedMargins
      = page_layout::Utils::constrainHardMarginsMM(contentRectMm, requestedMargins, QSizeF(100.0, 200.0));

  QPolygonF outputRect(contentRectMm);
  page_layout::Utils::extendPolyRectWithMargins(outputRect, constrainedMargins);
  const QRectF outputBounds(outputRect.boundingRect());

  CHECK(outputBounds.width() <= 100.0);
  CHECK(outputBounds.height() <= 200.0);
  CHECK(constrainedMargins.left() < requestedMargins.left());
  CHECK(constrainedMargins.top() < requestedMargins.top());
}

TEST_CASE("PageLayoutSettingsTestSuite / hard margins are constrained when content defines common size") {
  const QPolygonF contentRectMm{QPointF(10.0, 10.0), QPointF(90.0, 10.0), QPointF(90.0, 190.0), QPointF(10.0, 190.0)};
  const Margins requestedMargins(30.0, 30.0, 30.0, 30.0);

  const Margins constrainedMargins
      = page_layout::Utils::constrainHardMarginsMM(contentRectMm, requestedMargins, QSizeF(80.0, 180.0));

  QPolygonF outputRect(contentRectMm);
  page_layout::Utils::extendPolyRectWithMargins(outputRect, constrainedMargins);
  const QRectF outputBounds(outputRect.boundingRect());

  CHECK(outputBounds.width() <= 80.0);
  CHECK(outputBounds.height() <= 180.0);
  CHECK(constrainedMargins.left() < requestedMargins.left());
  CHECK(constrainedMargins.top() < requestedMargins.top());

  const Margins horizontalLimitedMargins
      = page_layout::Utils::constrainHardMarginsMM(contentRectMm, Margins(30.0, 2.0, 30.0, 2.0), QSizeF(100.0, 200.0));
  CHECK(horizontalLimitedMargins.top() == 2.0);
  CHECK(horizontalLimitedMargins.bottom() == 2.0);
}

TEST_CASE("PageLayoutSettingsTestSuite / content placement margins remain adjustable within Page Box size") {
  const QPolygonF contentRectMm{QPointF(0.0, 0.0), QPointF(40.0, 0.0), QPointF(40.0, 40.0), QPointF(0.0, 40.0)};
  const Margins requestedMargins(20.0, 10.0, 30.0, 20.0);

  const Margins constrainedMargins
      = page_layout::Utils::constrainHardMarginsMM(contentRectMm, requestedMargins, QSizeF(100.0, 100.0));

  CHECK(constrainedMargins.left() == requestedMargins.left());
  CHECK(constrainedMargins.top() == requestedMargins.top());
  CHECK(constrainedMargins.right() == requestedMargins.right());
  CHECK(constrainedMargins.bottom() == requestedMargins.bottom());
}

TEST_CASE("PageLayoutSettingsTestSuite / Page Box size stays fixed while margins reposition content") {
  const ImageTransformation xform(QRectF(0.0, 0.0, 200.0, 200.0), Dpi(254, 254));
  const QRectF pageRect(0.0, 0.0, 100.0, 100.0);
  const QRectF contentRect(20.0, 20.0, 40.0, 40.0);
  const QPolygonF contentRectPhys(xform.transformBack().map(QPolygonF(contentRect)));
  const page_layout::Alignment topLeftAlignment(page_layout::Alignment::TOP, page_layout::Alignment::LEFT);
  const page_layout::Params firstParams(Margins(1.0, 1.0, 3.0, 1.0), pageRect, contentRect, QSizeF(4.0, 4.0),
                                        topLeftAlignment, false, true, QSizeF(10.0, 10.0));
  const page_layout::Params secondParams(Margins(2.0, 2.0, 2.0, 2.0), pageRect, contentRect, QSizeF(4.0, 4.0),
                                         topLeftAlignment, false, true, QSizeF(10.0, 10.0));

  const QRectF firstOutputBounds(
      page_layout::Utils::calcPageRectPhys(xform, contentRectPhys, firstParams, QSizeF(10.0, 10.0)).boundingRect());
  const QRectF secondOutputBounds(
      page_layout::Utils::calcPageRectPhys(xform, contentRectPhys, secondParams, QSizeF(10.0, 10.0)).boundingRect());

  CHECK(firstOutputBounds.width() > 99.9);
  CHECK(firstOutputBounds.width() < 100.1);
  CHECK(secondOutputBounds.width() > 99.9);
  CHECK(secondOutputBounds.width() < 100.1);
  CHECK(firstOutputBounds.left() - secondOutputBounds.left() > 9.9);
  CHECK(firstOutputBounds.left() - secondOutputBounds.left() < 10.1);
}

TEST_CASE("PageLayoutSettingsTestSuite / extend_poly_rect_includes_other_rectangle") {
  QPolygonF outputRect{QPointF(0.0, 0.0), QPointF(10.0, 0.0), QPointF(10.0, 10.0), QPointF(0.0, 10.0)};
  const QPolygonF pageBoundary{QPointF(-5.0, -10.0), QPointF(15.0, -10.0), QPointF(15.0, 20.0), QPointF(-5.0, 20.0)};

  page_layout::Utils::extendPolyRectToInclude(outputRect, pageBoundary);

  CHECK(outputRect.boundingRect() == QRectF(-5.0, -10.0, 20.0, 30.0));
}

TEST_CASE("PageLayoutSettingsTestSuite / size ordering uses a stable snapshot during sorting") {
  const auto settings = std::make_shared<page_layout::Settings>();
  const PageId first(ImageId("/first.tif", 0), PageId::SINGLE_PAGE);
  const PageId middle(ImageId("/middle.tif", 0), PageId::SINGLE_PAGE);
  const PageId last(ImageId("/last.tif", 0), PageId::SINGLE_PAGE);
  settings->setContentSizeMM(first, QSizeF(10.0, 80.0));
  settings->setContentSizeMM(middle, QSizeF(20.0, 20.0));
  settings->setContentSizeMM(last, QSizeF(80.0, 10.0));
  for (const PageId& pageId : {first, middle, last}) {
    settings->setHardMarginsMM(pageId, Margins(0.0, 0.0, 0.0, 0.0));
  }

  const auto widthProvider = std::make_shared<page_layout::OrderByWidthProvider>(settings);
  const auto heightProvider = std::make_shared<page_layout::OrderByHeightProvider>(settings);
  const auto widthSnapshot = widthProvider->snapshot();
  const auto heightSnapshot = heightProvider->snapshot();
  const auto reversedWidthSnapshot = widthProvider->reversed()->snapshot();

  const auto mutateSettings = [&settings, &first, &middle, &last](const bool reverse) {
    if (reverse) {
      settings->setContentSizeMM(first, QSizeF(100.0, 10.0));
      settings->setContentSizeMM(middle, QSizeF(50.0, 100.0));
      settings->setContentSizeMM(last, QSizeF(1.0, 50.0));
    } else {
      settings->setContentSizeMM(first, QSizeF(1.0, 100.0));
      settings->setContentSizeMM(middle, QSizeF(100.0, 10.0));
      settings->setContentSizeMM(last, QSizeF(50.0, 50.0));
    }
  };
  const auto sortWhileSettingsChange
      = [&mutateSettings](const std::shared_ptr<const PageOrderProvider>& provider, std::vector<PageId> pages) {
          int comparisonCount = 0;
          std::sort(pages.begin(), pages.end(), [&](const PageId& lhs, const PageId& rhs) {
            mutateSettings(comparisonCount++ % 2 == 0);
            return provider->precedes(lhs, false, rhs, false);
          });
          return pages;
        };

  const std::vector<PageId> initialOrder{last, first, middle};
  const auto widthOrder = sortWhileSettingsChange(widthSnapshot, initialOrder);
  CHECK(widthOrder[0] == first);
  CHECK(widthOrder[1] == middle);
  CHECK(widthOrder[2] == last);

  const auto heightOrder = sortWhileSettingsChange(heightSnapshot, initialOrder);
  CHECK(heightOrder[0] == last);
  CHECK(heightOrder[1] == middle);
  CHECK(heightOrder[2] == first);

  const auto reversedWidthOrder = sortWhileSettingsChange(reversedWidthSnapshot, initialOrder);
  CHECK(reversedWidthOrder[0] == last);
  CHECK(reversedWidthOrder[1] == middle);
  CHECK(reversedWidthOrder[2] == first);
}
