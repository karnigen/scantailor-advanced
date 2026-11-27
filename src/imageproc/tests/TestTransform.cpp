// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#include <Grayscale.h>
#include <Transform.h>

#include <QImage>
#include <QSize>
#include <catch2/catch_test_macros.hpp>
#include <cmath>
#include <cstdint>
#include <cstdlib>

#include "Utils.h"

namespace imageproc {
namespace tests {
using namespace utils;


TEST_CASE("TransformTestSuite / test_null_image") {
  const QImage nullImg;
  const QTransform nullXform;
  const QRect unitRect(0, 0, 1, 1);
  const QColor bgcolor(0xff, 0xff, 0xff);
  const OutsidePixels outsidePixels(OutsidePixels::assumeColor(bgcolor));
  CHECK(transformToGray(nullImg, nullXform, unitRect, outsidePixels).isNull());
}

TEST_CASE("TransformTestSuite / test_random_image") {
  GrayImage img(QSize(100, 100));
  uint8_t* line = img.data();
  for (int y = 0; y < img.height(); ++y) {
    for (int x = 0; x < img.width(); ++x) {
      line[x] = static_cast<uint8_t>(rand() % 256);
    }
    line += img.stride();
  }

  const QColor bgcolor(0xff, 0xff, 0xff);
  const OutsidePixels outsidePixels(OutsidePixels::assumeColor(bgcolor));

  const QTransform nullXform;
  CHECK(transformToGray(img, nullXform, img.rect(), outsidePixels) == img);
}

}  // namespace tests
}  // namespace imageproc