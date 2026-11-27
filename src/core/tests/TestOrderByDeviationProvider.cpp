// SPDX-FileCopyrightText: 2026 ScanTailor Advanced contributors
// SPDX-License-Identifier: GPL-3.0-only

#include <DeviationProvider.h>
#include <ImageId.h>
#include <OrderByDeviationProvider.h>
#include <PageId.h>

#include <algorithm>
#include <catch2/catch_test_macros.hpp>
#include <vector>

TEST_CASE("OrderByDeviationProviderTestSuite / sort snapshot stays stable while values change") {
  DeviationProvider<PageId> deviationProvider;
  const PageId first(ImageId("/first.tif", 0));
  const PageId middle(ImageId("/middle.tif", 0));
  const PageId last(ImageId("/last.tif", 0));
  deviationProvider.addOrUpdate(first, 1.0);
  deviationProvider.addOrUpdate(middle, 10.0);
  deviationProvider.addOrUpdate(last, 50.0);

  const OrderByDeviationProvider orderProvider(deviationProvider);
  const std::shared_ptr<const PageOrderProvider> snapshot = orderProvider.snapshot();
  std::vector<PageId> pages{middle, last, first};
  int comparisons = 0;
  std::sort(pages.begin(), pages.end(), [&](const PageId& lhs, const PageId& rhs) {
    switch (comparisons++ % 3) {
      case 0:
        deviationProvider.addOrUpdate(first, 100.0);
        deviationProvider.addOrUpdate(middle, 10.0);
        deviationProvider.addOrUpdate(last, 1.0);
        break;
      case 1:
        deviationProvider.addOrUpdate(first, 1.0);
        deviationProvider.addOrUpdate(middle, 100.0);
        deviationProvider.addOrUpdate(last, 10.0);
        break;
      default:
        deviationProvider.addOrUpdate(first, 10.0);
        deviationProvider.addOrUpdate(middle, 1.0);
        deviationProvider.addOrUpdate(last, 100.0);
        break;
    }
    return snapshot->precedes(lhs, false, rhs, false);
  });

  CHECK(pages[0] == last);
  CHECK(pages[1] == first);
  CHECK(pages[2] == middle);
}
