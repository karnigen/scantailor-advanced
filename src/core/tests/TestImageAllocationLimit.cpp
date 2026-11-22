// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include <QFile>
#include <QImage>
#include <QImageReader>
#include <QReadLocker>
#include <QTemporaryDir>
#include <boost/test/unit_test.hpp>

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

  ~AllocationLimitRestorer() { ImageAllocationLimit::setLimitMb(m_limitMb); }

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

}  // namespace

BOOST_GLOBAL_FIXTURE(ClearImageAllocationLimitEnvironment);

BOOST_AUTO_TEST_SUITE(CoreImageAllocationLimitTestSuite)

BOOST_AUTO_TEST_CASE(test_environment_override_requires_nonnegative_integer) {
  qunsetenv("QT_IMAGEIO_MAXALLOC");
  BOOST_CHECK(!ImageAllocationLimit::hasEnvironmentOverride());

  qputenv("QT_IMAGEIO_MAXALLOC", QByteArray());
  BOOST_CHECK(!ImageAllocationLimit::hasEnvironmentOverride());

  qputenv("QT_IMAGEIO_MAXALLOC", QByteArrayLiteral("invalid"));
  BOOST_CHECK(!ImageAllocationLimit::hasEnvironmentOverride());

  qputenv("QT_IMAGEIO_MAXALLOC", QByteArrayLiteral("-1"));
  BOOST_CHECK(!ImageAllocationLimit::hasEnvironmentOverride());

  qputenv("QT_IMAGEIO_MAXALLOC", QByteArrayLiteral("0"));
  BOOST_CHECK(ImageAllocationLimit::hasEnvironmentOverride());

  qputenv("QT_IMAGEIO_MAXALLOC", QByteArrayLiteral("1024"));
  BOOST_CHECK(ImageAllocationLimit::hasEnvironmentOverride());
  qunsetenv("QT_IMAGEIO_MAXALLOC");
}

BOOST_AUTO_TEST_CASE(test_allocation_limit_is_capped) {
  AllocationLimitRestorer restoreLimit;
  BOOST_CHECK_EQUAL(ImageAllocationLimit::setLimitMb(32), 32);
  BOOST_CHECK_EQUAL(ImageAllocationLimit::setLimitMb(image_allocation_limit::kMaxLimitMb + 1), 32);

  ImageAllocationLimitInfo info;
  info.exceeded = true;
  info.requiredLimitMb = image_allocation_limit::kMaxLimitMb + 1;
  BOOST_CHECK(!info.canRaiseLimit());
}

BOOST_AUTO_TEST_CASE(test_saved_image_allocation_limit_rejects_out_of_range_values) {
  ApplicationSettings& settings = ApplicationSettings::getInstance();
  BOOST_CHECK(!settings.setImageAllocationLimitMb(-1));
  BOOST_CHECK(!settings.setImageAllocationLimitMb(image_allocation_limit::kMaxLimitMb + 1));
}

BOOST_AUTO_TEST_CASE(test_try_set_limit_distinguishes_busy_invalid_and_applied) {
  AllocationLimitRestorer restoreLimit;
  BOOST_CHECK_EQUAL(ImageAllocationLimit::setLimitMb(32), 32);

  int effectiveLimitMb = -1;
  BOOST_CHECK(ImageAllocationLimit::trySetLimitMb(-1, &effectiveLimitMb)
              == ImageAllocationLimit::TrySetResult::Invalid);
  BOOST_CHECK_EQUAL(effectiveLimitMb, -1);
  {
    QReadLocker allocationLimitLock(&ImageAllocationLimit::readWriteLock());
    BOOST_CHECK(ImageAllocationLimit::trySetLimitMb(64, &effectiveLimitMb) == ImageAllocationLimit::TrySetResult::Busy);
    BOOST_CHECK_EQUAL(effectiveLimitMb, -1);
  }

  BOOST_CHECK(ImageAllocationLimit::trySetLimitMb(64, &effectiveLimitMb)
              == ImageAllocationLimit::TrySetResult::Applied);
  BOOST_CHECK_EQUAL(effectiveLimitMb, 64);
}

BOOST_AUTO_TEST_CASE(test_image_loader_resets_limit_info_when_file_open_fails) {
  QTemporaryDir tempDir;
  BOOST_REQUIRE(tempDir.isValid());

  ImageAllocationLimitInfo info;
  info.exceeded = true;
  info.requiredLimitMb = 10;

  const QImage loaded = ImageLoader::load(tempDir.filePath("missing.png"), 0, &info);
  BOOST_CHECK(loaded.isNull());
  BOOST_CHECK(!info.exceeded);
  BOOST_CHECK_EQUAL(info.requiredLimitMb, 0);
}

BOOST_AUTO_TEST_CASE(test_detects_image_larger_than_limit) {
  AllocationLimitRestorer restoreLimit;
  BOOST_CHECK_EQUAL(ImageAllocationLimit::setLimitMb(1), 1);
  QReadLocker allocationLimitLock(&ImageAllocationLimit::readWriteLock());

  ImageAllocationLimitInfo info;
  BOOST_CHECK(ImageAllocationLimit::exceeds(QSize(513, 512), 32, &info));
  BOOST_CHECK(info.exceeded);
  BOOST_CHECK(info.imageSize == QSize(513, 512));
  BOOST_CHECK_EQUAL(info.currentLimitMb, 1);
  BOOST_CHECK_EQUAL(info.requiredLimitMb, 2);
  BOOST_CHECK(info.canRaiseLimit());
  BOOST_CHECK(ImageAllocationLimit::exceeds(QSize(513, 512), 32, nullptr));
  BOOST_CHECK(!ImageAllocationLimit::exceeds(QSize(512, 512), 32, nullptr));
}

BOOST_AUTO_TEST_CASE(test_accepts_image_within_limit) {
  AllocationLimitRestorer restoreLimit;
  ImageAllocationLimit::setLimitMb(1);
  QReadLocker allocationLimitLock(&ImageAllocationLimit::readWriteLock());

  ImageAllocationLimitInfo info;
  BOOST_CHECK(!ImageAllocationLimit::exceeds(QSize(512, 512), 32, &info));
  BOOST_CHECK(!info.exceeded);
}

BOOST_AUTO_TEST_CASE(test_unknown_dimensions_do_not_request_limit) {
  AllocationLimitRestorer restoreLimit;
  ImageAllocationLimit::setLimitMb(1);
  QReadLocker allocationLimitLock(&ImageAllocationLimit::readWriteLock());

  ImageAllocationLimitInfo info;
  BOOST_CHECK(!ImageAllocationLimit::exceeds(QSize(), 32, &info));
  BOOST_CHECK(!info.dimensionsKnown);
  BOOST_CHECK(!info.exceeded);
  BOOST_CHECK_EQUAL(info.requiredLimitMb, 0);
}

BOOST_AUTO_TEST_CASE(test_rejects_image_exceeding_representable_limit_without_wrapping) {
  AllocationLimitRestorer restoreLimit;
  ImageAllocationLimit::setLimitMb(1);
  QReadLocker allocationLimitLock(&ImageAllocationLimit::readWriteLock());

  ImageAllocationLimitInfo info;
  BOOST_CHECK(ImageAllocationLimit::exceeds(QSize(INT_MAX, INT_MAX), 32, &info));
  BOOST_CHECK(info.exceeded);
  BOOST_CHECK_EQUAL(info.requiredLimitMb, 0);
  BOOST_CHECK(!info.canRaiseLimit());
}

BOOST_AUTO_TEST_CASE(test_image_loader_reports_oversized_png_before_decoding) {
  QTemporaryDir tempDir;
  BOOST_REQUIRE(tempDir.isValid());

  const QString imagePath(tempDir.filePath("large.png"));
  QImage source(QSize(513, 512), QImage::Format_ARGB32);
  source.fill(Qt::white);
  BOOST_REQUIRE(source.save(imagePath, "PNG"));

  AllocationLimitRestorer restoreLimit;
  ImageAllocationLimit::setLimitMb(1);

  ImageAllocationLimitInfo info;
  const QImage loaded = ImageLoader::load(imagePath, 0, &info);
  BOOST_CHECK(loaded.isNull());
  BOOST_CHECK(info.exceeded);
  BOOST_CHECK_EQUAL(info.requiredLimitMb, 2);
  BOOST_CHECK(ImageLoader::load(imagePath).isNull());
}

BOOST_AUTO_TEST_CASE(test_image_loader_uses_16_bit_png_depth_for_limit_estimate) {
  QTemporaryDir tempDir;
  BOOST_REQUIRE(tempDir.isValid());

  const QString imagePath(tempDir.filePath("16-bit.png"));
  QImage source(QSize(512, 512), QImage::Format_RGBA64);
  source.fill(Qt::white);
  BOOST_REQUIRE(source.save(imagePath, "PNG"));

  AllocationLimitRestorer restoreLimit;
  ImageAllocationLimit::setLimitMb(1);

  ImageAllocationLimitInfo info;
  const QImage loaded = ImageLoader::load(imagePath, 0, &info);
  BOOST_CHECK(loaded.isNull());
  BOOST_CHECK(info.exceeded);
  BOOST_CHECK_EQUAL(info.requiredLimitMb, 2);
}

BOOST_AUTO_TEST_CASE(test_tiff_loader_checks_limit_before_allocating_and_loads_after_increase) {
  QTemporaryDir tempDir;
  BOOST_REQUIRE(tempDir.isValid());

  const QString imagePath(tempDir.filePath("large.tif"));
  BOOST_REQUIRE(writeRgbTiff(imagePath, QSize(513, 512)));

  AllocationLimitRestorer restoreLimit;
  ImageAllocationLimit::setLimitMb(1);

  ImageAllocationLimitInfo info;
  const QImage rejected = ImageLoader::load(imagePath, 0, &info);
  BOOST_CHECK(rejected.isNull());
  BOOST_CHECK(info.exceeded);
  BOOST_CHECK_EQUAL(info.requiredLimitMb, 2);

  ImageAllocationLimit::setLimitMb(info.requiredLimitMb);
  const QImage loaded = ImageLoader::load(imagePath);
  BOOST_CHECK(loaded.size() == QSize(513, 512));
}

BOOST_AUTO_TEST_SUITE_END()
