// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#include <ImageId.h>
#include <PageId.h>

#include <QString>
#include <catch2/catch_test_macros.hpp>


TEST_CASE("CorePageIdTestSuite / test_default_constructor_is_null") {
  const PageId id;
  CHECK(id.isNull());
}

TEST_CASE("CorePageIdTestSuite / test_subpage_to_string") {
  CHECK(PageId::subPageToString(PageId::SINGLE_PAGE) == "single");
  CHECK(PageId::subPageToString(PageId::LEFT_PAGE) == "left");
  CHECK(PageId::subPageToString(PageId::RIGHT_PAGE) == "right");
}

TEST_CASE("CorePageIdTestSuite / test_subpage_from_string") {
  bool ok = false;
  CHECK(PageId::subPageFromString("single", &ok) == PageId::SINGLE_PAGE);
  CHECK(ok);
  CHECK(PageId::subPageFromString("left", &ok) == PageId::LEFT_PAGE);
  CHECK(ok);
  CHECK(PageId::subPageFromString("right", &ok) == PageId::RIGHT_PAGE);
  CHECK(ok);

  PageId::subPageFromString("invalid", &ok);
  CHECK(!ok);
  CHECK(PageId::subPageFromString("", &ok) == PageId::SINGLE_PAGE);
  CHECK(!ok);
}

TEST_CASE("CorePageIdTestSuite / test_constructor_with_image_id") {
  const ImageId imageId("/path/to/image.tiff", 0);
  const PageId pageId(imageId, PageId::LEFT_PAGE);
  CHECK(!pageId.isNull());
  CHECK(pageId.imageId() == imageId);
  CHECK(pageId.subPage() == PageId::LEFT_PAGE);
  CHECK(pageId.subPageAsString() == "left");
}

TEST_CASE("CorePageIdTestSuite / test_equality") {
  const ImageId img1("/a", 0);
  const ImageId img2("/b", 0);
  const PageId p1(img1, PageId::SINGLE_PAGE);
  const PageId p2(img1, PageId::SINGLE_PAGE);
  const PageId p3(img2, PageId::SINGLE_PAGE);
  const PageId p4(img1, PageId::LEFT_PAGE);

  CHECK(p1 == p2);
  CHECK(p1 != p3);
  CHECK(p1 != p4);
}

TEST_CASE("CorePageIdTestSuite / test_ordering") {
  const ImageId img1("/a", 0);
  const ImageId img2("/b", 0);
  const PageId p1(img1, PageId::SINGLE_PAGE);
  const PageId p2(img1, PageId::LEFT_PAGE);
  const PageId p3(img1, PageId::RIGHT_PAGE);
  const PageId p4(img2, PageId::SINGLE_PAGE);

  CHECK(p1 < p2);
  CHECK(p2 < p3);
  CHECK(p3 < p4);
  CHECK(!(p1 < p1));
}
