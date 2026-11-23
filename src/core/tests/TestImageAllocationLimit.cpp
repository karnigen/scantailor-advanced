// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include <QFile>
#include <QImage>
#include <QImageReader>
#include <QReadLocker>
#include <QTemporaryDir>
#include <catch2/catch_test_macros.hpp>

#include <tiffio.h>
#include <climits>
#include <cstdint>
#include <vector>

#include "ApplicationSettings.h"
#include "ImageAllocationLimit.h"
#include "ImageLoader.h"

namespace {

class ClearImageAllocationLimitEnvironment {
 public:
  ClearImageAllocationLimitEnvironment() { qunsetenv("QT_IMAGEIO_MAXALLOC"); }
};

class AllocationLimitRestorer {
 public:
  AllocationLimitRestorer() : m_limitMb(QImageReader::allocationLimit()) {}

  ~AllocationLimitRestorer() {
    int effectiveLimitMb = 0;
    ImageAllocationLimit::setLimitMb(m_limitMb, &effectiveLimitMb);
  }

 private:
  int m_limitMb;
};

bool writeRgbTiff(const QString& path, const QSize& size) {
  const QByteArray encodedPath(QFile::encodeName(path));
  TIFF* const tiff = TIFFOpen(encodedPath.constData(), "w");
  if (!tiff) {
    return false;
  }

  bool success = TIFFSetField(tiff, TIFFTAG_IMAGEWIDTH, static_cast<uint32_t>(size.width()))
                 && TIFFSetField(tiff, TIFFTAG_IMAGELENGTH, static_cast<uint32_t>(size.height()))
                 && TIFFSetField(tiff, TIFFTAG_SAMPLESPERPIXEL, 3) && TIFFSetField(tiff, TIFFTAG_BITSPERSAMPLE, 8)
                 && TIFFSetField(tiff, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT)
                 && TIFFSetField(tiff, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG)
                 && TIFFSetField(tiff, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_RGB)
                 && TIFFSetField(tiff, TIFFTAG_COMPRESSION, COMPRESSION_NONE)
                 && TIFFSetField(tiff, TIFFTAG_ROWSPERSTRIP, static_cast<uint32_t>(size.height()));
  std::vector<uint8_t> row(static_cast<size_t>(size.width()) * 3);
  for (int y = 0; success && y < size.height(); ++y) {
    success = TIFFWriteScanline(tiff, row.data(), static_cast<uint32_t>(y), 0) >= 0;
  }
  TIFFClose(tiff);
  return success;
}

bool setImageAllocationLimitForTest(const int limitMb) {
  int effectiveLimitMb = -1;
  return ImageAllocationLimit::setLimitMb(limitMb, &effectiveLimitMb) == ImageAllocationLimit::UpdateResult::Applied
         && effectiveLimitMb == limitMb;
}

}  // namespace
static const ClearImageAllocationLimitEnvironment clearImageAllocationLimitEnvironment;
TEST_CASE("CoreImageAllocationLimitTestSuite / test_environment_override_requires_nonnegative_integer") {
  qunsetenv("QT_IMAGEIO_MAXALLOC");
  CHECK(!ImageAllocationLimit::hasEnvironmentOverride());

  qputenv("QT_IMAGEIO_MAXALLOC", QByteArray());
  CHECK(!ImageAllocationLimit::hasEnvironmentOverride());

  qputenv("QT_IMAGEIO_MAXALLOC", QByteArrayLiteral("invalid"));
  CHECK(!ImageAllocationLimit::hasEnvironmentOverride());

  qputenv("QT_IMAGEIO_MAXALLOC", QByteArrayLiteral("-1"));
  CHECK(!ImageAllocationLimit::hasEnvironmentOverride());

  qputenv("QT_IMAGEIO_MAXALLOC", QByteArrayLiteral("0"));
  CHECK(ImageAllocationLimit::hasEnvironmentOverride());

  qputenv("QT_IMAGEIO_MAXALLOC", QByteArrayLiteral("1024"));
  CHECK(ImageAllocationLimit::hasEnvironmentOverride());
  qunsetenv("QT_IMAGEIO_MAXALLOC");
}

TEST_CASE("CoreImageAllocationLimitTestSuite / test_allocation_limit_is_capped") {
  AllocationLimitRestorer restoreLimit;
  CHECK(setImageAllocationLimitForTest(32));
  int effectiveLimitMb = 32;
  CHECK(ImageAllocationLimit::setLimitMb(image_allocation_limit::kMaxLimitMb + 1, &effectiveLimitMb)
        == ImageAllocationLimit::UpdateResult::Invalid);
  CHECK((effectiveLimitMb) == (32));

  ImageAllocationLimitInfo info;
  info.exceeded = true;
  info.requiredLimitMb = image_allocation_limit::kMaxLimitMb + 1;
  CHECK(!info.canRaiseLimit());
}

TEST_CASE("CoreImageAllocationLimitTestSuite / test_saved_image_allocation_limit_rejects_out_of_range_values") {
  ApplicationSettings& settings = ApplicationSettings::getInstance();
  CHECK(!settings.setImageAllocationLimitMb(-1));
  CHECK(!settings.setImageAllocationLimitMb(image_allocation_limit::kMaxLimitMb + 1));
}

TEST_CASE("CoreImageAllocationLimitTestSuite / test_try_set_limit_distinguishes_busy_invalid_and_applied") {
  AllocationLimitRestorer restoreLimit;
  CHECK(setImageAllocationLimitForTest(32));

  int effectiveLimitMb = -1;
  CHECK(ImageAllocationLimit::trySetLimitMb(-1, &effectiveLimitMb) == ImageAllocationLimit::UpdateResult::Invalid);
  CHECK((effectiveLimitMb) == (-1));
  {
    QReadLocker allocationLimitLock(&ImageAllocationLimit::readWriteLock());
    CHECK(ImageAllocationLimit::trySetLimitMb(64, &effectiveLimitMb) == ImageAllocationLimit::UpdateResult::Busy);
    CHECK((effectiveLimitMb) == (-1));
  }

  CHECK(ImageAllocationLimit::trySetLimitMb(64, &effectiveLimitMb) == ImageAllocationLimit::UpdateResult::Applied);
  CHECK((effectiveLimitMb) == (64));
}

TEST_CASE("CoreImageAllocationLimitTestSuite / test_image_loader_resets_limit_info_when_file_open_fails") {
  QTemporaryDir tempDir;
  REQUIRE(tempDir.isValid());

  ImageAllocationLimitInfo info;
  info.exceeded = true;
  info.requiredLimitMb = 10;

  const QImage loaded = ImageLoader::load(tempDir.filePath("missing.png"), 0, &info);
  CHECK(loaded.isNull());
  CHECK(!info.exceeded);
  CHECK((info.requiredLimitMb) == (0));
}

TEST_CASE("CoreImageAllocationLimitTestSuite / test_detects_image_larger_than_limit") {
  AllocationLimitRestorer restoreLimit;
  CHECK(setImageAllocationLimitForTest(1));
  QReadLocker allocationLimitLock(&ImageAllocationLimit::readWriteLock());

  ImageAllocationLimitInfo info;
  CHECK(ImageAllocationLimit::exceeds(QSize(513, 512), 32, &info));
  CHECK(info.exceeded);
  CHECK(info.imageSize == QSize(513, 512));
  CHECK((info.currentLimitMb) == (1));
  CHECK((info.requiredLimitMb) == (2));
  CHECK(info.canRaiseLimit());
  CHECK(ImageAllocationLimit::exceeds(QSize(513, 512), 32, nullptr));
  CHECK(!ImageAllocationLimit::exceeds(QSize(512, 512), 32, nullptr));
}

TEST_CASE("CoreImageAllocationLimitTestSuite / test_accepts_image_within_limit") {
  AllocationLimitRestorer restoreLimit;
  CHECK(setImageAllocationLimitForTest(1));
  QReadLocker allocationLimitLock(&ImageAllocationLimit::readWriteLock());

  ImageAllocationLimitInfo info;
  CHECK(!ImageAllocationLimit::exceeds(QSize(512, 512), 32, &info));
  CHECK(!info.exceeded);
}

TEST_CASE("CoreImageAllocationLimitTestSuite / test_unknown_dimensions_do_not_request_limit") {
  AllocationLimitRestorer restoreLimit;
  CHECK(setImageAllocationLimitForTest(1));
  QReadLocker allocationLimitLock(&ImageAllocationLimit::readWriteLock());

  ImageAllocationLimitInfo info;
  CHECK(!ImageAllocationLimit::exceeds(QSize(), 32, &info));
  CHECK(!info.dimensionsKnown);
  CHECK(!info.exceeded);
  CHECK((info.requiredLimitMb) == (0));
}

TEST_CASE("CoreImageAllocationLimitTestSuite / test_rejects_image_exceeding_representable_limit_without_wrapping") {
  AllocationLimitRestorer restoreLimit;
  CHECK(setImageAllocationLimitForTest(1));
  QReadLocker allocationLimitLock(&ImageAllocationLimit::readWriteLock());

  ImageAllocationLimitInfo info;
  CHECK(ImageAllocationLimit::exceeds(QSize(INT_MAX, INT_MAX), 32, &info));
  CHECK(info.exceeded);
  CHECK((info.requiredLimitMb) == (0));
  CHECK(!info.canRaiseLimit());
}

TEST_CASE("CoreImageAllocationLimitTestSuite / test_image_loader_reports_oversized_png_before_decoding") {
  QTemporaryDir tempDir;
  REQUIRE(tempDir.isValid());

  const QString imagePath(tempDir.filePath("large.png"));
  QImage source(QSize(513, 512), QImage::Format_ARGB32);
  source.fill(Qt::white);
  REQUIRE(source.save(imagePath, "PNG"));

  AllocationLimitRestorer restoreLimit;
  CHECK(setImageAllocationLimitForTest(1));

  ImageAllocationLimitInfo info;
  const QImage loaded = ImageLoader::load(imagePath, 0, &info);
  CHECK(loaded.isNull());
  CHECK(info.exceeded);
  CHECK((info.requiredLimitMb) == (2));
  CHECK(ImageLoader::load(imagePath).isNull());
}

TEST_CASE("CoreImageAllocationLimitTestSuite / test_image_loader_uses_16_bit_png_depth_for_limit_estimate") {
  QTemporaryDir tempDir;
  REQUIRE(tempDir.isValid());

  const QString imagePath(tempDir.filePath("16-bit.png"));
  QImage source(QSize(512, 512), QImage::Format_RGBA64);
  source.fill(Qt::white);
  REQUIRE(source.save(imagePath, "PNG"));

  AllocationLimitRestorer restoreLimit;
  CHECK(setImageAllocationLimitForTest(1));

  ImageAllocationLimitInfo info;
  const QImage loaded = ImageLoader::load(imagePath, 0, &info);
  CHECK(loaded.isNull());
  CHECK(info.exceeded);
  CHECK((info.requiredLimitMb) == (2));
}

TEST_CASE(
    "CoreImageAllocationLimitTestSuite / test_tiff_loader_checks_limit_before_allocating_and_loads_after_increase") {
  QTemporaryDir tempDir;
  REQUIRE(tempDir.isValid());

  const QString imagePath(tempDir.filePath("large.tif"));
  REQUIRE(writeRgbTiff(imagePath, QSize(513, 512)));

  AllocationLimitRestorer restoreLimit;
  CHECK(setImageAllocationLimitForTest(1));

  ImageAllocationLimitInfo info;
  const QImage rejected = ImageLoader::load(imagePath, 0, &info);
  CHECK(rejected.isNull());
  CHECK(info.exceeded);
  CHECK((info.requiredLimitMb) == (2));

  CHECK(setImageAllocationLimitForTest(info.requiredLimitMb));
  const QImage loaded = ImageLoader::load(imagePath);
  CHECK(loaded.size() == QSize(513, 512));
}
