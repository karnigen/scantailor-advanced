// Copyright (C) 2026  ScanTailor Advanced contributors
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "../filters/page_layout/CacheDrivenTask.h"
#include "../filters/page_layout/Params.h"
#include "../filters/page_layout/Settings.h"

#include <Dpi.h>
#include <ImageId.h>
#include <ImageTransformation.h>
#include <PageId.h>
#include <PageInfo.h>

#include <QRectF>
#include <QSizeF>
#include <catch2/catch_test_macros.hpp>
#include <memory>

TEST_CASE("PageLayoutCacheDrivenTaskTestSuite / preview geometry does not overwrite worker settings") {
  const auto settings = std::make_shared<page_layout::Settings>();
  const PageId pageId(ImageId("/page.tif", 0), PageId::SINGLE_PAGE);
  const QRectF workerPageRect(0.0, 0.0, 200.0, 300.0);
  const QRectF workerContentRect(20.0, 30.0, 160.0, 250.0);
  const QSizeF workerContentSize(160.0, 250.0);
  settings->setPageParams(pageId, page_layout::Params(Margins(5.0, 5.0, 5.0, 5.0), workerPageRect, workerContentRect,
                                                      workerContentSize, page_layout::Alignment(), false));

  PageInfo pageInfo;
  pageInfo.setId(pageId);
  const ImageTransformation xform(QRectF(0.0, 0.0, 100.0, 100.0), Dpi(300, 300));
  page_layout::CacheDrivenTask task(nullptr, settings);
  task.process(pageInfo, nullptr, xform, QRectF(0.0, 0.0, 120.0, 120.0), QRectF(15.0, 15.0, 90.0, 90.0));

  const std::unique_ptr<page_layout::Params> currentParams = settings->getPageParams(pageId);
  REQUIRE(currentParams);
  CHECK(currentParams->pageRect() == workerPageRect);
  CHECK(currentParams->contentRect() == workerContentRect);
  CHECK(currentParams->contentSizeMM() == workerContentSize);
}
