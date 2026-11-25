// Copyright (C) 2026  ScanTailor Advanced contributors
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "../filters/page_layout/Alignment.h"
#include "../filters/page_layout/OrderByHeightProvider.h"
#include "../filters/page_layout/OrderByWidthProvider.h"
#include "../filters/page_layout/Settings.h"

#include <ImageId.h>
#include <PageId.h>

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
