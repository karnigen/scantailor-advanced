// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#include <ImageId.h>
#include <PageId.h>
#include <PageRange.h>

#include <catch2/catch_test_macros.hpp>
#include <set>


TEST_CASE("CorePageRangeTestSuite / test_select_every_other_empty_range") {
  PageRange range;
  const PageId base(ImageId("/x", 0), PageId::SINGLE_PAGE);
  const std::set<PageId> result = range.selectEveryOther(base);
  CHECK(result.empty());
}

TEST_CASE("CorePageRangeTestSuite / test_select_every_other_base_not_in_range") {
  PageRange range;
  range.pages.push_back(PageId(ImageId("/a", 0), PageId::SINGLE_PAGE));
  range.pages.push_back(PageId(ImageId("/b", 0), PageId::SINGLE_PAGE));
  const PageId base(ImageId("/c", 0), PageId::SINGLE_PAGE);
  const std::set<PageId> result = range.selectEveryOther(base);
  CHECK(result.empty());
}

TEST_CASE("CorePageRangeTestSuite / test_select_every_other_alternates") {
  PageRange range;
  const PageId p0(ImageId("/a", 0), PageId::SINGLE_PAGE);
  const PageId p1(ImageId("/b", 0), PageId::SINGLE_PAGE);
  const PageId p2(ImageId("/c", 0), PageId::SINGLE_PAGE);
  const PageId p3(ImageId("/d", 0), PageId::SINGLE_PAGE);
  range.pages = {p0, p1, p2, p3};

  std::set<PageId> result = range.selectEveryOther(p0);
  CHECK((result.size()) == (2u));
  CHECK(result.count(p0) == 1);
  CHECK(result.count(p2) == 1);
  CHECK(result.count(p1) == 0);
  CHECK(result.count(p3) == 0);

  result = range.selectEveryOther(p1);
  CHECK((result.size()) == (2u));
  CHECK(result.count(p1) == 1);
  CHECK(result.count(p3) == 1);
}

TEST_CASE("CorePageRangeTestSuite / test_select_every_other_single_page") {
  PageRange range;
  const PageId p(ImageId("/x", 0), PageId::SINGLE_PAGE);
  range.pages.push_back(p);
  const std::set<PageId> result = range.selectEveryOther(p);
  CHECK((result.size()) == (1u));
  CHECK(result.count(p) == 1);
}
