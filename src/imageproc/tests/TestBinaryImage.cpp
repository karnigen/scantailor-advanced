// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#include <BWColor.h>
#include <BinaryImage.h>

#include <QImage>
#include <catch2/catch_test_macros.hpp>
#include <cstdlib>

#include "Utils.h"

namespace imageproc {
namespace tests {
using namespace utils;


TEST_CASE("BinaryImageTestSuite / test_null_image") {
  CHECK(BinaryImage().toQImage() == QImage());
  CHECK(BinaryImage(QImage()).toQImage() == QImage());
}

TEST_CASE("BinaryImageTestSuite / test_from_to_qimage") {
  const int w = 50;
  const int h = 64;
  QImage qimgArgb32(w, h, QImage::Format_ARGB32);
  QImage qimgMono(w, h, QImage::Format_Mono);
  qimgMono.setColorCount(2);
  qimgMono.setColor(0, 0xffffffff);
  qimgMono.setColor(1, 0xff000000);
  for (int y = 0; y < h; ++y) {
    for (int x = 0; x < w; ++x) {
      const int rnd = rand() & 1;
      qimgArgb32.setPixel(x, y, rnd ? 0x66888888 : 0x66777777);
      qimgMono.setPixel(x, y, rnd ? 0 : 1);
    }
  }

  QImage qimgMonoLsb(qimgMono.convertToFormat(QImage::Format_MonoLSB));
  QImage qimgRgb32(qimgArgb32.convertToFormat(QImage::Format_RGB32));
  QImage qimgArgb32Pm(qimgArgb32.convertToFormat(QImage::Format_ARGB32_Premultiplied));
  QImage qimgRgb16(qimgRgb32.convertToFormat(QImage::Format_RGB16));
  QImage qimgIndexed8(qimgRgb32.convertToFormat(QImage::Format_Indexed8));

  REQUIRE(BinaryImage(qimgMono).toQImage() == qimgMono);
  CHECK(BinaryImage(qimgMonoLsb).toQImage() == qimgMono);
  CHECK(BinaryImage(qimgArgb32).toQImage() == qimgMono);
  CHECK(BinaryImage(qimgRgb32).toQImage() == qimgMono);
  CHECK(BinaryImage(qimgArgb32Pm).toQImage() == qimgMono);
  CHECK(BinaryImage(qimgIndexed8).toQImage() == qimgMono);

  // A bug in Qt prevents this from working.
  // CHECK(BinaryImage(qimgRgb16, 0x80).toQImage() == qimgMono);
}

TEST_CASE("BinaryImageTestSuite / test_full_fill") {
  BinaryImage white(100, 100);
  white.fill(WHITE);

  QImage qWhite(100, 100, QImage::Format_Mono);
  qWhite.fill(1);

  REQUIRE(BinaryImage(qWhite) == white);

  BinaryImage black(30, 30);
  black.fill(BLACK);

  QImage qBlack(30, 30, QImage::Format_Mono);
  qBlack.fill(0);

  REQUIRE(BinaryImage(qBlack) == black);
}

TEST_CASE("BinaryImageTestSuite / test_partial_fill_small") {
  QImage qImage(randomMonoQImage(100, 100));

  const QRect rect(80, 80, 20, 20);
  BinaryImage image(qImage);
  image.fill(rect, WHITE);
  QImage whiteRect(rect.width(), rect.height(), QImage::Format_Mono);
  whiteRect.setColorCount(2);
  whiteRect.setColor(0, 0xffffffff);
  whiteRect.setColor(1, 0xff000000);
  whiteRect.fill(0);
  REQUIRE(image.toQImage().copy(rect) == whiteRect);
  CHECK(surroundingsIntact(image.toQImage(), qImage, rect));
}

TEST_CASE("BinaryImageTestSuite / test_partial_fill_large") {
  QImage qImage(randomMonoQImage(100, 100));

  const QRect rect(20, 20, 79, 79);
  BinaryImage image(qImage);
  image.fill(rect, WHITE);
  QImage whiteRect(rect.width(), rect.height(), QImage::Format_Mono);
  whiteRect.setColorCount(2);
  whiteRect.setColor(0, 0xffffffff);
  whiteRect.setColor(1, 0xff000000);
  whiteRect.fill(0);
  REQUIRE(image.toQImage().copy(rect) == whiteRect);
  CHECK(surroundingsIntact(image.toQImage(), qImage, rect));
}

TEST_CASE("BinaryImageTestSuite / test_fill_except") {
  QImage qImage(randomMonoQImage(100, 100));

  const QRect rect(20, 20, 79, 79);
  BinaryImage image(qImage);
  image.fillExcept(rect, BLACK);

  QImage blackImage(qImage.width(), qImage.height(), QImage::Format_Mono);
  blackImage.setColorCount(2);
  blackImage.setColor(0, 0xffffffff);
  blackImage.setColor(1, 0xff000000);
  blackImage.fill(1);

  REQUIRE(image.toQImage().copy(rect) == qImage.copy(rect));
  CHECK(surroundingsIntact(image.toQImage(), blackImage, rect));
}

TEST_CASE("BinaryImageTestSuite / test_content_bounding_box4") {
  static const int inp[]
      = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0,
         0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

  const BinaryImage img(makeBinaryImage(inp, 9, 8));
  CHECK(img.contentBoundingBox() == QRect(1, 1, 6, 6));
}

}  // namespace tests
}  // namespace imageproc