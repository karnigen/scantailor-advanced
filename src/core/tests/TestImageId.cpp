// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include <ImageId.h>
#include <PathUtils.h>
#include <catch2/catch_test_macros.hpp>
#include <filesystem>


TEST_CASE("CoreImageIdTestSuite / test_default_constructor_is_null") {
  const ImageId id;
  CHECK(id.isNull());
  CHECK(id.filePath().empty());
  CHECK(id.page() == 0);
}

TEST_CASE("CoreImageIdTestSuite / test_constructor_with_path") {
  const ImageId id("/path/to/file.png", 0);
  CHECK(!id.isNull());
  CHECK(id.filePath() == "/path/to/file.png");
  CHECK(id.page() == 0);
  CHECK(!id.isMultiPageFile());
  CHECK(id.zeroBasedPage() == 0);
}

TEST_CASE("CoreImageIdTestSuite / test_multipage") {
  const ImageId id("/file.tiff", 3);
  CHECK(id.page() == 3);
  CHECK(id.isMultiPageFile());
  CHECK(id.zeroBasedPage() == 2);
}

TEST_CASE("CoreImageIdTestSuite / test_equality") {
  const ImageId a("/x", 1);
  const ImageId b("/x", 1);
  const ImageId c("/y", 1);
  const ImageId d("/x", 2);
  CHECK(a == b);
  CHECK(a != c);
  CHECK(a != d);
}

TEST_CASE("CoreImageIdTestSuite / test_ordering") {
  const ImageId a("/a", 0);
  const ImageId b("/b", 0);
  const ImageId c("/a", 1);
  CHECK(a < b);
  CHECK(a < c);
  CHECK(c < b);
}

TEST_CASE("CoreImageIdTestSuite / test_from_qfileinfo") {
  const ImageId id(std::filesystem::path("/absolute/path/image.jpg"), 0);
  CHECK(id.filePath() == "/absolute/path/image.jpg");
}

TEST_CASE("CoreImageIdTestSuite / test_path_identity_preserves_dot_components") {
  const ImageId withDot("images/./page.png");
  const ImageId withoutDot("images/page.png");
  CHECK(withDot != withoutDot);
}

TEST_CASE("CoreImageIdTestSuite / test_qstring_path_conversion_preserves_unicode") {
  const QString path = QStringLiteral("složka/obrázek.tif");
  CHECK(core::toQString(core::toFileSystemPath(path)) == path);
}

TEST_CASE("CoreImageIdTestSuite / test_setters") {
  ImageId id;
  id.setFilePath("/new/path");
  id.setPage(2);
  CHECK(id.filePath() == "/new/path");
  CHECK(id.page() == 2);
}
