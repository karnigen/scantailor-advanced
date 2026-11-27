// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#include <BinaryImage.h>
#include <ReduceThreshold.h>

#include <QImage>
#include <catch2/catch_test_macros.hpp>

#include "Utils.h"

namespace imageproc {
namespace tests {
using namespace utils;


TEST_CASE("ReduceThresholdTestSuite / test_null_image") {
  CHECK(ReduceThreshold(BinaryImage())(2).image().isNull());
}

TEST_CASE("ReduceThresholdTestSuite / test_small_image") {
  const int inp[] = {0, 0, 1, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,

                     0, 0, 1, 0, 0, 1, 1, 1, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0,

                     0, 0, 1, 0, 0, 1, 1, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1,

                     0, 0, 1, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};
  const int out1[] = {0, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};
  const int out2[] = {0, 0, 0, 1, 0, 0, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1};
  const int out3[] = {0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 1, 1, 1, 1};
  const int out4[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1};

  const BinaryImage img(makeBinaryImage(inp, 10, 8));

  CHECK(makeBinaryImage(out1, 5, 4) == ReduceThreshold(img)(1));
  CHECK(makeBinaryImage(out2, 5, 4) == ReduceThreshold(img)(2));
  CHECK(makeBinaryImage(out3, 5, 4) == ReduceThreshold(img)(3));
  CHECK(makeBinaryImage(out4, 5, 4) == ReduceThreshold(img)(4));
}

TEST_CASE("ReduceThresholdTestSuite / test_lines") {
  static const int inp[] = {0, 0, 0, 1, 1, 0, 1, 1, 0};
  static const int out1[] = {0, 1, 1, 1};
  static const int out2[] = {0, 1, 1, 1};
  static const int out3[] = {0, 0, 0, 1};
  static const int out4[] = {0, 0, 0, 1};

  BinaryImage img(makeBinaryImage(inp, 9, 1));

  CHECK(makeBinaryImage(out1, 4, 1) == ReduceThreshold(img)(1));
  CHECK(makeBinaryImage(out2, 4, 1) == ReduceThreshold(img)(2));
  CHECK(makeBinaryImage(out3, 4, 1) == ReduceThreshold(img)(3));
  CHECK(makeBinaryImage(out4, 4, 1) == ReduceThreshold(img)(4));

  img = makeBinaryImage(inp, 1, 9);

  CHECK(makeBinaryImage(out1, 1, 4) == ReduceThreshold(img)(1));
  CHECK(makeBinaryImage(out2, 1, 4) == ReduceThreshold(img)(2));
  CHECK(makeBinaryImage(out3, 1, 4) == ReduceThreshold(img)(3));
  CHECK(makeBinaryImage(out4, 1, 4) == ReduceThreshold(img)(4));
}

}  // namespace tests
}  // namespace imageproc