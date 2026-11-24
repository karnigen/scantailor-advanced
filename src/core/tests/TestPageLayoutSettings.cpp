// Copyright (C) 2026  ScanTailor Advanced contributors
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "../filters/page_layout/Alignment.h"
#include "../filters/page_layout/Params.h"
#include "../filters/page_layout/Settings.h"

#include <ImageId.h>
#include <PageId.h>

#include <QSizeF>
#include <catch2/catch_test_macros.hpp>


TEST_CASE("PageLayoutSettingsTestSuite / test_aggregate_dimensions_track_independent_maxima") {
  page_layout::Settings settings;
  const PageId first(ImageId("/first.tif", 0), PageId::SINGLE_PAGE);
  const PageId second(ImageId("/second.tif", 0), PageId::SINGLE_PAGE);

  settings.setContentSizeMM(first, QSizeF(100.0, 50.0));
  settings.setContentSizeMM(second, QSizeF(200.0, 20.0));
  CHECK(settings.getAggregateHardSizeMM() == QSizeF(220.0, 60.0));

  settings.setContentSizeMM(second, QSizeF(50.0, 100.0));
  CHECK(settings.getAggregateHardSizeMM() == QSizeF(120.0, 110.0));

  settings.setHardMarginsMM(first, Margins(0.0, 0.0, 0.0, 0.0));
  CHECK(settings.getAggregateHardSizeMM() == QSizeF(100.0, 110.0));

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
  CHECK(settings.getAggregateHardSizeMM() == QSizeF(120.0, 60.0));
}

TEST_CASE("PageLayoutSettingsTestSuite / stale thumbnail snapshot cannot overwrite newer worker geometry") {
  page_layout::Settings settings;
  const PageId pageId(ImageId("/page.tif", 0), PageId::SINGLE_PAGE);
  const page_layout::Params initialParams(Margins(5.0, 5.0, 5.0, 5.0), QRectF(0.0, 0.0, 100.0, 100.0),
                                          QRectF(10.0, 10.0, 80.0, 80.0), QSizeF(80.0, 80.0), page_layout::Alignment(),
                                          false);
  settings.setPageParams(pageId, initialParams);

  const std::unique_ptr<page_layout::Params> thumbnailSnapshot = settings.getPageParams(pageId);
  REQUIRE(thumbnailSnapshot);

  const QRectF thumbnailPageRect(0.0, 0.0, 110.0, 110.0);
  const QRectF thumbnailContentRect(12.0, 12.0, 86.0, 86.0);
  const page_layout::Params thumbnailParams = settings.updateContentSizeAndGetParamsIfUnchanged(
      pageId, thumbnailPageRect, thumbnailContentRect, QSizeF(86.0, 86.0), *thumbnailSnapshot);
  CHECK(thumbnailParams.pageRect() == thumbnailPageRect);

  const std::unique_ptr<page_layout::Params> staleSnapshot = settings.getPageParams(pageId);
  REQUIRE(staleSnapshot);
  const QRectF workerPageRect(0.0, 0.0, 200.0, 300.0);
  const QRectF workerContentRect(20.0, 30.0, 160.0, 250.0);
  const page_layout::Params workerParams
      = settings.updateContentSizeAndGetParams(pageId, workerPageRect, workerContentRect, QSizeF(160.0, 250.0));

  const page_layout::Params returnedParams = settings.updateContentSizeAndGetParamsIfUnchanged(
      pageId, staleSnapshot->pageRect(), staleSnapshot->contentRect(), staleSnapshot->contentSizeMM(), *staleSnapshot);
  const std::unique_ptr<page_layout::Params> currentParams = settings.getPageParams(pageId);
  REQUIRE(currentParams);
  CHECK(returnedParams.pageRect() == workerPageRect);
  CHECK(returnedParams.contentRect() == workerContentRect);
  CHECK(returnedParams.contentSizeMM() == QSizeF(160.0, 250.0));
  CHECK(currentParams->pageRect() == workerParams.pageRect());
  CHECK(currentParams->contentRect() == workerParams.contentRect());
  CHECK(currentParams->contentSizeMM() == workerParams.contentSizeMM());
}
