// SPDX-FileCopyrightText: 2026 ScanTailor Advanced contributors
// SPDX-License-Identifier: GPL-3.0-only

#include <ImageId.h>
#include <PageId.h>
#include <filters/page_split/OrderBySplitTypeProvider.h>
#include <filters/page_split/Settings.h>
#include <filters/select_content/Dependencies.h>
#include <filters/select_content/OrderByWidthProvider.h>
#include <filters/select_content/Params.h>
#include <filters/select_content/Settings.h>

#include <QSizeF>
#include <algorithm>
#include <catch2/catch_test_macros.hpp>
#include <memory>
#include <set>
#include <vector>

namespace {
std::vector<PageId> sortPages(const PageOrderProvider& provider, std::vector<PageId> pages) {
  std::sort(pages.begin(), pages.end(),
            [&provider](const PageId& lhs, const PageId& rhs) { return provider.precedes(lhs, false, rhs, false); });
  return pages;
}

void setContentSize(select_content::Settings& settings, const PageId& pageId, const QSizeF& contentSizeMM) {
  select_content::Params params((select_content::Dependencies()));
  params.setContentSizeMM(contentSizeMM);
  settings.setPageParams(pageId, params);
}
}  // namespace

TEST_CASE("OrderProviderSnapshotTestSuite / select content width snapshot keeps the order it was taken with") {
  const auto settings = std::make_shared<select_content::Settings>();
  const PageId narrow(ImageId("/narrow.tif", 0), PageId::SINGLE_PAGE);
  const PageId medium(ImageId("/medium.tif", 0), PageId::SINGLE_PAGE);
  const PageId wide(ImageId("/wide.tif", 0), PageId::SINGLE_PAGE);
  setContentSize(*settings, narrow, QSizeF(10.0, 100.0));
  setContentSize(*settings, medium, QSizeF(50.0, 50.0));
  setContentSize(*settings, wide, QSizeF(100.0, 10.0));

  const auto provider = std::make_shared<select_content::OrderByWidthProvider>(settings);
  const std::vector<PageId> pages{narrow, medium, wide};
  const std::vector<PageId> orderWhenSnapshotted = sortPages(*provider, pages);
  REQUIRE(orderWhenSnapshotted == std::vector<PageId>({narrow, medium, wide}));

  const std::shared_ptr<const PageOrderProvider> snapshot = provider->snapshot();
  const std::shared_ptr<const PageOrderProvider> reversedSnapshot = provider->reversed()->snapshot();

  // Page sizes changing mid-pass must not affect an ordering pass already in progress.
  setContentSize(*settings, narrow, QSizeF(70.0, 100.0));
  setContentSize(*settings, medium, QSizeF(20.0, 50.0));
  setContentSize(*settings, wide, QSizeF(90.0, 10.0));

  CHECK(sortPages(*snapshot, pages) == orderWhenSnapshotted);
  CHECK(sortPages(*reversedSnapshot, pages) == std::vector<PageId>({wide, medium, narrow}));
  CHECK(sortPages(*provider, pages) == std::vector<PageId>({medium, narrow, wide}));
}

TEST_CASE("OrderProviderSnapshotTestSuite / page split type snapshot keeps the order it was taken with") {
  const auto settings = std::make_shared<page_split::Settings>();
  const PageId first(ImageId("/first.tif", 0), PageId::SINGLE_PAGE);
  const PageId second(ImageId("/second.tif", 0), PageId::SINGLE_PAGE);
  const PageId third(ImageId("/third.tif", 0), PageId::SINGLE_PAGE);

  settings->setLayoutTypeFor(page_split::SINGLE_PAGE_UNCUT, {first});
  settings->setLayoutTypeFor(page_split::TWO_PAGES, {second, third});

  const auto provider = std::make_shared<page_split::OrderBySplitTypeProvider>(settings);
  const std::vector<PageId> pages{third, second, first};
  const std::vector<PageId> orderWhenSnapshotted = sortPages(*provider, pages);
  REQUIRE(orderWhenSnapshotted == std::vector<PageId>({first, second, third}));

  const std::shared_ptr<const PageOrderProvider> snapshot = provider->snapshot();

  settings->setLayoutTypeFor(page_split::TWO_PAGES, {first});
  settings->setLayoutTypeFor(page_split::SINGLE_PAGE_UNCUT, {third});

  CHECK(sortPages(*snapshot, pages) == orderWhenSnapshotted);
  CHECK(sortPages(*provider, pages) == std::vector<PageId>({third, first, second}));
}
