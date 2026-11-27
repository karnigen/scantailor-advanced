// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#include <Grayscale.h>

#include <QImage>
#include <catch2/catch_test_macros.hpp>
#include <cstdlib>

#include "Utils.h"

namespace imageproc {
namespace tests {
using namespace utils;


TEST_CASE("GrayscaleTestSuite / test_null_image") {
  CHECK(toGrayscale(QImage()).isNull());
}

TEST_CASE("GrayscaleTestSuite / test_mono_to_grayscale") {
  const int w = 50;
  const int h = 64;

  QImage mono(w, h, QImage::Format_Mono);
  QImage gray(w, h, QImage::Format_Indexed8);
  gray.setColorTable(createGrayscalePalette());

  for (int y = 0; y < h; ++y) {
    for (int x = 0; x < w; ++x) {
      const int rnd = rand() & 1;
      mono.setPixel(x, y, rnd ? 0 : 1);
      gray.setPixel(x, y, rnd ? 0 : 255);
    }
  }

  const QImage monoLsb(mono.convertToFormat(QImage::Format_MonoLSB));

  REQUIRE(toGrayscale(mono) == gray);
  CHECK(toGrayscale(monoLsb) == gray);
}

TEST_CASE("GrayscaleTestSuite / test_argb32_to_grayscale") {
  const int w = 50;
  const int h = 64;
  QImage argb32(w, h, QImage::Format_ARGB32);
  QImage gray(w, h, QImage::Format_Indexed8);
  gray.setColorTable(createGrayscalePalette());

  for (int y = 0; y < h; ++y) {
    for (int x = 0; x < w; ++x) {
      const int rnd = rand() & 1;
      argb32.setPixel(x, y, rnd ? 0x80303030 : 0x80606060);
      gray.setPixel(x, y, rnd ? 0x30 : 0x60);
    }
  }

  CHECK(toGrayscale(argb32) == gray);
}

TEST_CASE("GrayscaleTestSuite / color formats match per-pixel grayscale conversion") {
  const int width = 37;
  const int height = 23;
  QImage argb(width, height, QImage::Format_ARGB32);
  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      argb.setPixel(
          x, y,
          qRgba((x * 17 + y * 3) & 0xff, (x * 5 + y * 29) & 0xff, (x * 31 + y * 7) & 0xff, 0x80 | ((x + y) & 0x7f)));
    }
  }

  const QImage::Format formats[] = {QImage::Format_RGB32, QImage::Format_ARGB32, QImage::Format_RGB888,
                                    QImage::Format_ARGB32_Premultiplied, QImage::Format_RGB16};
  for (const QImage::Format format : formats) {
    const QImage src(argb.convertToFormat(format));
    const QImage gray(toGrayscale(src));

    REQUIRE(gray.format() == QImage::Format_Indexed8);
    REQUIRE(gray.size() == src.size());
    for (int y = 0; y < height; ++y) {
      for (int x = 0; x < width; ++x) {
        CHECK(gray.constScanLine(y)[x] == qGray(src.pixel(x, y)));
      }
    }
  }
}

}  // namespace tests
}  // namespace imageproc