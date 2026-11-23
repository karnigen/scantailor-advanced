// Copyright (C) 2026  ScanTailor Advanced contributors
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "../filters/page_layout/Alignment.h"
#include "../filters/page_layout/Settings.h"

#include <ImageId.h>
#include <PageId.h>

#include <QSizeF>
#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(PageLayoutSettingsTestSuite)

BOOST_AUTO_TEST_CASE(test_aggregate_dimensions_track_independent_maxima) {
  page_layout::Settings settings;
  const PageId first(ImageId("/first.tif", 0), PageId::SINGLE_PAGE);
  const PageId second(ImageId("/second.tif", 0), PageId::SINGLE_PAGE);

  settings.setContentSizeMM(first, QSizeF(100.0, 50.0));
  settings.setContentSizeMM(second, QSizeF(200.0, 20.0));
  BOOST_CHECK(settings.getAggregateHardSizeMM() == QSizeF(220.0, 60.0));

  settings.setContentSizeMM(second, QSizeF(50.0, 100.0));
  BOOST_CHECK(settings.getAggregateHardSizeMM() == QSizeF(120.0, 110.0));

  settings.setHardMarginsMM(first, Margins(0.0, 0.0, 0.0, 0.0));
  BOOST_CHECK(settings.getAggregateHardSizeMM() == QSizeF(100.0, 110.0));

  page_layout::Alignment unaligned;
  unaligned.setNull(true);
  settings.setPageAlignment(second, unaligned);
  BOOST_CHECK(settings.getAggregateHardSizeMM() == QSizeF(100.0, 50.0));

  settings.setPageAlignment(first, unaligned);
  BOOST_CHECK(settings.getAggregateHardSizeMM() == QSizeF(0.0, 0.0));
}

BOOST_AUTO_TEST_CASE(test_equal_dimensions_keep_both_ordered_entries) {
  page_layout::Settings settings;
  const PageId first(ImageId("/first.tif", 0), PageId::SINGLE_PAGE);
  const PageId second(ImageId("/second.tif", 0), PageId::SINGLE_PAGE);
  settings.setContentSizeMM(first, QSizeF(100.0, 50.0));
  settings.setContentSizeMM(second, QSizeF(100.0, 50.0));

  page_layout::Alignment unaligned;
  unaligned.setNull(true);
  settings.setPageAlignment(first, unaligned);
  BOOST_CHECK(settings.getAggregateHardSizeMM() == QSizeF(120.0, 60.0));
}

BOOST_AUTO_TEST_SUITE_END()
