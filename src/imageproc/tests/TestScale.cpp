// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#include <GrayImage.h>
#include <Scale.h>

#include <QSize>
#include <algorithm>
#include <array>
#include <catch2/catch_test_macros.hpp>
#include <cstdint>
#include <cstdlib>

namespace imageproc {
namespace tests {

static GrayImage makeGrayImageFromPixels(const QSize& size, const uint8_t* pixels) {
  GrayImage image(size);
  for (int y = 0; y < size.height(); ++y) {
    uint8_t* row = image.data() + y * image.stride();
    for (int x = 0; x < size.width(); ++x) {
      row[x] = pixels[y * size.width() + x];
    }
  }
  return image;
}

static GrayImage makeNoiseImage() {
  std::array<uint8_t, 100 * 100> pixels{};
  uint32_t state = 0x12345678;
  for (uint8_t& pixel : pixels) {
    state = state * 1664525u + 1013904223u;
    pixel = static_cast<uint8_t>(state >> 24);
  }
  return makeGrayImageFromPixels(QSize(100, 100), pixels.data());
}

// Exact source-pixel overlap weights in destination-coordinate units.
static int areaAverage(const GrayImage& image, const QSize& size, const int x, const int y) {
  const int left = x * image.width();
  const int right = (x + 1) * image.width();
  const int top = y * image.height();
  const int bottom = (y + 1) * image.height();
  int weightedSum = 0;
  for (int sy = top / size.height(); sy < (bottom + size.height() - 1) / size.height(); ++sy) {
    const int weightY = std::min(bottom, (sy + 1) * size.height()) - std::max(top, sy * size.height());
    for (int sx = left / size.width(); sx < (right + size.width() - 1) / size.width(); ++sx) {
      const int weightX = std::min(right, (sx + 1) * size.width()) - std::max(left, sx * size.width());
      weightedSum += image.data()[sy * image.stride() + sx] * weightX * weightY;
    }
  }
  const int totalWeight = image.width() * image.height();
  return (weightedSum + totalWeight / 2) / totalWeight;
}

TEST_CASE("ScaleTestSuite / test_null_image") {
  const GrayImage nullImg;
  CHECK(scaleToGray(nullImg, QSize(1, 1)).isNull());
}

TEST_CASE("ScaleTestSuite / test_downscale_integer_ratio_averages_source_blocks") {
  const uint8_t pixels[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
  const GrayImage image = makeGrayImageFromPixels(QSize(4, 4), pixels);
  const GrayImage scaled = scaleToGray(image, QSize(2, 2));
  const uint8_t expected[] = {3, 5, 11, 13};

  for (int y = 0; y < 2; ++y) {
    const uint8_t* row = scaled.data() + y * scaled.stride();
    for (int x = 0; x < 2; ++x) {
      CHECK(row[x] == expected[y * 2 + x]);
    }
  }
}

TEST_CASE("ScaleTestSuite / test_downscale_fractional_ratio_averages_source_coverage") {
  const uint8_t pixels[] = {1, 2, 3, 4, 5, 6, 7, 8, 9};
  const GrayImage image = makeGrayImageFromPixels(QSize(3, 3), pixels);
  const GrayImage scaled = scaleToGray(image, QSize(2, 2));
  const uint8_t expected[] = {2, 4, 6, 8};

  for (int y = 0; y < 2; ++y) {
    const uint8_t* row = scaled.data() + y * scaled.stride();
    for (int x = 0; x < 2; ++x) {
      CHECK(row[x] == expected[y * 2 + x]);
    }
  }
}

TEST_CASE("ScaleTestSuite / test_integer_upscale_with_different_axis_factors") {
  const uint8_t pixels[] = {17, 83, 145, 231};
  const GrayImage image = makeGrayImageFromPixels(QSize(2, 2), pixels);

  for (const QSize size : {QSize(2, 4), QSize(4, 2), QSize(4, 6), QSize(6, 4)}) {
    const GrayImage scaled = scaleToGray(image, size);
    REQUIRE(scaled.size() == size);
    const int xscale = size.width() / image.width();
    const int yscale = size.height() / image.height();
    for (int y = 0; y < size.height(); ++y) {
      const uint8_t* row = scaled.data() + y * scaled.stride();
      for (int x = 0; x < size.width(); ++x) {
        CAPTURE(size, x, y);
        CHECK(row[x] == pixels[(y / yscale) * image.width() + x / xscale]);
      }
    }
  }
}

TEST_CASE("ScaleTestSuite / test_upscale_with_single_pixel_source_axis") {
  SECTION("single source column") {
    const uint8_t pixels[] = {20, 100};
    const GrayImage image = makeGrayImageFromPixels(QSize(1, 2), pixels);
    const GrayImage scaled = scaleToGray(image, QSize(3, 3));
    REQUIRE(scaled.size() == QSize(3, 3));

    int previous = 0;
    for (int y = 0; y < scaled.height(); ++y) {
      const uint8_t* row = scaled.data() + y * scaled.stride();
      CAPTURE(y, int(row[0]), int(row[1]), int(row[2]));
      CHECK(row[0] == row[1]);
      CHECK(row[1] == row[2]);
      CHECK(row[0] >= 20);
      CHECK(row[0] <= 100);
      CHECK(row[0] >= previous);
      previous = row[0];
    }
  }

  SECTION("single source row") {
    const uint8_t pixels[] = {20, 100};
    const GrayImage image = makeGrayImageFromPixels(QSize(2, 1), pixels);
    const GrayImage scaled = scaleToGray(image, QSize(3, 3));
    REQUIRE(scaled.size() == QSize(3, 3));

    const uint8_t* firstRow = scaled.data();
    for (int y = 1; y < scaled.height(); ++y) {
      const uint8_t* row = scaled.data() + y * scaled.stride();
      for (int x = 0; x < scaled.width(); ++x) {
        CAPTURE(x, y);
        CHECK(row[x] == firstRow[x]);
      }
    }
  }
}

TEST_CASE("ScaleTestSuite / test_downscale_high_contrast_against_area_reference") {
  const GrayImage image = makeNoiseImage();
  for (const QSize size : {QSize(50, 50), QSize(80, 80)}) {
    const GrayImage scaled = scaleToGray(image, size);
    REQUIRE(scaled.size() == size);
    for (int y = 0; y < size.height(); ++y) {
      const uint8_t* row = scaled.data() + y * scaled.stride();
      for (int x = 0; x < size.width(); ++x) {
        CAPTURE(size, x, y);
        CHECK(row[x] == areaAverage(image, size, x, y));
      }
    }
  }
}

struct QtDifference {
  int maxDifference = 0;
  int pixelsAboveOne = 0;
  int totalDifference = 0;
};

static QtDifference compareWithQt(const GrayImage& image, const QSize& size, const QImage::Format format) {
  const GrayImage actual = scaleToGray(image, size);
  const QImage qtScaled
      = image.toQImage().convertToFormat(format).scaled(size, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
  const GrayImage qt(qtScaled);
  REQUIRE(actual.size() == size);
  REQUIRE(qt.size() == size);

  QtDifference difference;
  for (int y = 0; y < size.height(); ++y) {
    for (int x = 0; x < size.width(); ++x) {
      const int delta = std::abs(int(actual.data()[y * actual.stride() + x]) - int(qt.data()[y * qt.stride() + x]));
      difference.maxDifference = std::max(difference.maxDifference, delta);
      difference.pixelsAboveOne += delta > 1;
      difference.totalDifference += delta;
    }
  }
  return difference;
}

static void checkQtCompatibility(const GrayImage& image, const QSize& size, const QImage::Format format) {
  const QtDifference difference = compareWithQt(image, size, format);
  const double meanDifference = double(difference.totalDifference) / (size.width() * size.height());
  CAPTURE(qVersion(), format, size.width(), size.height(), difference.maxDifference, difference.pixelsAboveOne,
          meanDifference);
  CHECK(difference.maxDifference <= 1);
}

TEST_CASE("ScaleTestSuite / diagnostic Qt compatibility", "[.][scale-qt-diagnostic]") {
  const GrayImage image = makeNoiseImage();

  SECTION("Indexed8 input (original comparison)") {
    for (const QSize size : {QSize(50, 50), QSize(80, 80)}) {
      checkQtCompatibility(image, size, QImage::Format_Indexed8);
    }
  }

  SECTION("RGB32 input (Qt smoothScaled path)") {
    for (const QSize size : {QSize(50, 50), QSize(80, 80)}) {
      checkQtCompatibility(image, size, QImage::Format_RGB32);
    }
  }
}

}  // namespace tests
}  // namespace imageproc