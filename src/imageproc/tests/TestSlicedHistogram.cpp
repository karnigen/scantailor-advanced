// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#include <BinaryImage.h>
#include <SlicedHistogram.h>

#include <QImage>
#include <catch2/catch_test_macros.hpp>
#include <cstddef>
#include <stdexcept>

#include "Utils.h"

namespace imageproc {
namespace tests {
using namespace utils;

static bool checkHistogram(const SlicedHistogram& hist, const int* dataBegin, const int* dataEnd) {
  if (hist.size() != size_t(dataEnd - dataBegin)) {
    return false;
  }
  for (unsigned i = 0; i < hist.size(); ++i) {
    if (hist[i] != dataBegin[i]) {
      return false;
    }
  }
  return true;
}


TEST_CASE("SlicedHistogramTestSuite / test_null_image") {
  const BinaryImage nullImg;

  SlicedHistogram horHist(nullImg, SlicedHistogram::ROWS);
  CHECK(horHist.size() == 0);

  SlicedHistogram verHist(nullImg, SlicedHistogram::COLS);
  CHECK(verHist.size() == 0);
}

TEST_CASE("SlicedHistogramTestSuite / test_exceeding_area") {
  const BinaryImage img(1, 1);
  const QRect area(0, 0, 1, 2);

  CHECK_THROWS_AS((SlicedHistogram(img, area, SlicedHistogram::ROWS)), std::invalid_argument);
}

TEST_CASE("SlicedHistogramTestSuite / test_small_image") {
  static const int inp[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0,
                            1, 0, 0, 1, 1, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 1, 0, 0, 1, 0,
                            0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 1, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1};

  static const int hor_counts[] = {0, 1, 2, 3, 9, 2, 6, 3, 1};

  static const int ver_counts[] = {2, 2, 4, 4, 5, 3, 2, 3, 2};

  const BinaryImage img(makeBinaryImage(inp, 9, 9));

  SlicedHistogram horHist(img, SlicedHistogram::ROWS);
  CHECK(checkHistogram(horHist, hor_counts, hor_counts + 9));

  SlicedHistogram verHist(img, SlicedHistogram::COLS);
  CHECK(checkHistogram(verHist, ver_counts, ver_counts + 9));

  horHist = SlicedHistogram(img, img.rect().adjusted(0, 1, 0, 0), SlicedHistogram::ROWS);
  CHECK(checkHistogram(horHist, hor_counts + 1, hor_counts + 9));

  verHist = SlicedHistogram(img, img.rect().adjusted(1, 0, 0, 0), SlicedHistogram::COLS);
  CHECK(checkHistogram(verHist, ver_counts + 1, ver_counts + 9));
}

}  // namespace tests
}  // namespace imageproc