// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

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

}  // namespace tests
}  // namespace imageproc