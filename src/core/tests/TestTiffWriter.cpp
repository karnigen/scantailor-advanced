// SPDX-FileCopyrightText: 2026 project contributors
// SPDX-License-Identifier: GPL-3.0-only

#include <catch2/catch_test_macros.hpp>

#include <QFile>
#include <QImage>
#include <QTemporaryDir>

#include <TiffWriter.h>
#include <tiffio.h>
#include <memory>
#include <vector>

namespace {

struct TiffMetadata {
  uint16_t bitsPerSample = 0;
  uint16_t samplesPerPixel = 0;
  uint16_t photometric = 0;
  uint16_t compression = 0;
  uint16_t resolutionUnit = 0;
  float xResolution = 0;
  float yResolution = 0;
};

TiffMetadata readMetadata(const QString& path) {
  const QByteArray nativePath = QFile::encodeName(path);
  const std::unique_ptr<TIFF, decltype(&TIFFClose)> tiff(TIFFOpen(nativePath.constData(), "r"), TIFFClose);
  REQUIRE(tiff != nullptr);

  TiffMetadata metadata;
  REQUIRE(TIFFGetField(tiff.get(), TIFFTAG_BITSPERSAMPLE, &metadata.bitsPerSample));
  REQUIRE(TIFFGetField(tiff.get(), TIFFTAG_SAMPLESPERPIXEL, &metadata.samplesPerPixel));
  REQUIRE(TIFFGetField(tiff.get(), TIFFTAG_PHOTOMETRIC, &metadata.photometric));
  REQUIRE(TIFFGetField(tiff.get(), TIFFTAG_COMPRESSION, &metadata.compression));
  REQUIRE(TIFFGetField(tiff.get(), TIFFTAG_XRESOLUTION, &metadata.xResolution));
  REQUIRE(TIFFGetField(tiff.get(), TIFFTAG_YRESOLUTION, &metadata.yResolution));
  TIFFGetFieldDefaulted(tiff.get(), TIFFTAG_RESOLUTIONUNIT, &metadata.resolutionUnit);
  return metadata;
}

void setResolution(QImage& image, const int dpiX = 300, const int dpiY = 300) {
  image.setDotsPerMeterX(qRound(dpiX / 0.0254));
  image.setDotsPerMeterY(qRound(dpiY / 0.0254));
}

void checkPixelsMatch(const QImage& expected, const QString& path) {
  const QByteArray nativePath = QFile::encodeName(path);
  const std::unique_ptr<TIFF, decltype(&TIFFClose)> tiff(TIFFOpen(nativePath.constData(), "r"), TIFFClose);
  REQUIRE(tiff != nullptr);

  uint32_t width = 0;
  uint32_t height = 0;
  uint16_t bitsPerSample = 0;
  uint16_t samplesPerPixel = 0;
  uint16_t photometric = 0;
  REQUIRE(TIFFGetField(tiff.get(), TIFFTAG_IMAGEWIDTH, &width));
  REQUIRE(TIFFGetField(tiff.get(), TIFFTAG_IMAGELENGTH, &height));
  REQUIRE(TIFFGetField(tiff.get(), TIFFTAG_BITSPERSAMPLE, &bitsPerSample));
  REQUIRE(TIFFGetField(tiff.get(), TIFFTAG_SAMPLESPERPIXEL, &samplesPerPixel));
  REQUIRE(TIFFGetField(tiff.get(), TIFFTAG_PHOTOMETRIC, &photometric));
  REQUIRE(width == static_cast<uint32_t>(expected.width()));
  REQUIRE(height == static_cast<uint32_t>(expected.height()));

  const bool paletteLayout
      = photometric == PHOTOMETRIC_PALETTE && samplesPerPixel == 1 && (bitsPerSample == 1 || bitsPerSample == 8);
  const bool grayscaleLayout = (photometric == PHOTOMETRIC_MINISBLACK || photometric == PHOTOMETRIC_MINISWHITE)
                               && samplesPerPixel == 1 && (bitsPerSample == 1 || bitsPerSample == 8);
  const bool rgbLayout = photometric == PHOTOMETRIC_RGB && samplesPerPixel == 3 && bitsPerSample == 8;
  REQUIRE((paletteLayout || grayscaleLayout || rgbLayout));

  uint16_t* redMap = nullptr;
  uint16_t* greenMap = nullptr;
  uint16_t* blueMap = nullptr;
  if (photometric == PHOTOMETRIC_PALETTE) {
    REQUIRE(TIFFGetField(tiff.get(), TIFFTAG_COLORMAP, &redMap, &greenMap, &blueMap));
  }
  const tmsize_t scanlineSize = TIFFScanlineSize(tiff.get());
  REQUIRE(scanlineSize > 0);
  std::vector<uint8_t> scanline(static_cast<size_t>(scanlineSize));

  for (uint32_t y = 0; y < height; ++y) {
    REQUIRE(TIFFReadScanline(tiff.get(), scanline.data(), y, 0) >= 0);
    for (uint32_t x = 0; x < width; ++x) {
      uint8_t red = 0;
      uint8_t green = 0;
      uint8_t blue = 0;
      if (photometric == PHOTOMETRIC_PALETTE) {
        const uint16_t index
            = bitsPerSample == 1 ? static_cast<uint16_t>((scanline[x / 8] >> (7 - (x % 8))) & 1) : scanline[x];
        red = static_cast<uint8_t>(redMap[index] / 257);
        green = static_cast<uint8_t>(greenMap[index] / 257);
        blue = static_cast<uint8_t>(blueMap[index] / 257);
      } else if (photometric == PHOTOMETRIC_RGB) {
        const size_t offset = static_cast<size_t>(x) * samplesPerPixel;
        red = scanline[offset];
        green = scanline[offset + 1];
        blue = scanline[offset + 2];
      } else {
        const uint8_t value
            = bitsPerSample == 1 ? static_cast<uint8_t>((scanline[x / 8] >> (7 - (x % 8))) & 1) : scanline[x];
        const uint8_t gray = bitsPerSample == 1 ? static_cast<uint8_t>(value * 255) : value;
        red = gray;
        green = gray;
        blue = gray;
      }
      CHECK(expected.pixelColor(static_cast<int>(x), static_cast<int>(y)) == QColor(red, green, blue));
    }
  }
}

QString writeSourceTiff(const QTemporaryDir& directory,
                        const QString& fileName,
                        QImage& image,
                        const int dpiX = 300,
                        const int dpiY = 300) {
  setResolution(image, dpiX, dpiY);
  const QString path = directory.filePath(fileName);
  REQUIRE(TiffWriter::writeImage(path, image));
  return path;
}

void appendOnePixelRgbDirectory(const QString& path) {
  const QByteArray nativePath = QFile::encodeName(path);
  const std::unique_ptr<TIFF, decltype(&TIFFClose)> tiff(TIFFOpen(nativePath.constData(), "a"), TIFFClose);
  REQUIRE(tiff != nullptr);

  TIFFSetField(tiff.get(), TIFFTAG_IMAGEWIDTH, uint32_t{1});
  TIFFSetField(tiff.get(), TIFFTAG_IMAGELENGTH, uint32_t{1});
  TIFFSetField(tiff.get(), TIFFTAG_SAMPLESPERPIXEL, uint16_t{3});
  TIFFSetField(tiff.get(), TIFFTAG_BITSPERSAMPLE, uint16_t{8});
  TIFFSetField(tiff.get(), TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
  TIFFSetField(tiff.get(), TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_RGB);
  TIFFSetField(tiff.get(), TIFFTAG_COMPRESSION, COMPRESSION_NONE);
  uint8_t pixel[3] = {0, 0, 0};
  REQUIRE(TIFFWriteScanline(tiff.get(), pixel, 0, 0) >= 0);
  REQUIRE(TIFFWriteDirectory(tiff.get()));
}

}  // namespace

TEST_CASE("Rendered TIFF optimization stores exact black and white pixels as bilevel LZW", "[tiff-optimization]") {
  QTemporaryDir directory;
  REQUIRE(directory.isValid());
  QImage image(5, 3, QImage::Format_RGB32);
  image.fill(Qt::white);
  image.setPixelColor(0, 0, Qt::black);
  image.setPixelColor(2, 1, Qt::black);
  image.setPixelColor(4, 2, Qt::black);
  const QString path = writeSourceTiff(directory, QStringLiteral("bilevel.tif"), image);

  const auto result = TiffWriter::optimizeRenderedPdfPage(path, 300);
  CHECK(result.mode == TiffWriter::RenderedPageColorMode::Bitonal);
  const TiffMetadata metadata = readMetadata(path);
  CHECK(metadata.bitsPerSample == 1);
  CHECK(metadata.samplesPerPixel == 1);
  CHECK(metadata.photometric == PHOTOMETRIC_MINISBLACK);
  CHECK(metadata.compression == COMPRESSION_LZW);
  CHECK(metadata.xResolution > 299.0F);
  CHECK(metadata.xResolution < 301.0F);
  CHECK(metadata.yResolution > 299.0F);
  CHECK(metadata.yResolution < 301.0F);
  CHECK(metadata.resolutionUnit == RESUNIT_INCH);
  checkPixelsMatch(image, path);
}

TEST_CASE("Rendered TIFF optimization stores grayscale pages as 8-bit LZW", "[tiff-optimization]") {
  QTemporaryDir directory;
  REQUIRE(directory.isValid());
  QImage image(4, 2, QImage::Format_RGB32);
  image.setPixelColor(0, 0, QColor(0, 0, 0));
  image.setPixelColor(1, 0, QColor(64, 64, 64));
  image.setPixelColor(2, 0, QColor(192, 192, 192));
  image.setPixelColor(3, 0, QColor(255, 255, 255));
  image.setPixelColor(0, 1, QColor(20, 20, 20));
  image.setPixelColor(1, 1, QColor(80, 80, 80));
  image.setPixelColor(2, 1, QColor(160, 160, 160));
  image.setPixelColor(3, 1, QColor(240, 240, 240));
  const QString path = writeSourceTiff(directory, QStringLiteral("grayscale.tif"), image);

  const auto result = TiffWriter::optimizeRenderedPdfPage(path, 300);
  CHECK(result.mode == TiffWriter::RenderedPageColorMode::Grayscale);
  const TiffMetadata metadata = readMetadata(path);
  CHECK(metadata.bitsPerSample == 8);
  CHECK(metadata.samplesPerPixel == 1);
  CHECK(metadata.photometric == PHOTOMETRIC_MINISBLACK);
  CHECK(metadata.compression == COMPRESSION_LZW);
  checkPixelsMatch(image, path);
}

TEST_CASE("Rendered TIFF optimization uses 1-bit indexed color for two exact colors", "[tiff-optimization]") {
  QTemporaryDir directory;
  REQUIRE(directory.isValid());
  QImage image(5, 2, QImage::Format_RGB32);
  image.setPixelColor(0, 0, QColor(230, 20, 30));
  image.setPixelColor(1, 0, QColor(10, 170, 240));
  image.setPixelColor(2, 0, QColor(230, 20, 30));
  image.setPixelColor(3, 0, QColor(10, 170, 240));
  image.setPixelColor(4, 0, QColor(230, 20, 30));
  image.setPixelColor(0, 1, QColor(10, 170, 240));
  image.setPixelColor(1, 1, QColor(230, 20, 30));
  image.setPixelColor(2, 1, QColor(10, 170, 240));
  image.setPixelColor(3, 1, QColor(230, 20, 30));
  image.setPixelColor(4, 1, QColor(10, 170, 240));
  const QString path = writeSourceTiff(directory, QStringLiteral("two-colors.tif"), image, 400, 600);

  const auto result = TiffWriter::optimizeRenderedPdfPage(path, 300);
  CHECK(result.mode == TiffWriter::RenderedPageColorMode::IndexedColor);
  const TiffMetadata metadata = readMetadata(path);
  CHECK(metadata.bitsPerSample == 1);
  CHECK(metadata.samplesPerPixel == 1);
  CHECK(metadata.photometric == PHOTOMETRIC_PALETTE);
  CHECK(metadata.compression == COMPRESSION_LZW);
  CHECK(metadata.xResolution > 399.0F);
  CHECK(metadata.xResolution < 401.0F);
  CHECK(metadata.yResolution > 599.0F);
  CHECK(metadata.yResolution < 601.0F);
  checkPixelsMatch(image, path);
}

TEST_CASE("Rendered TIFF optimization indexes up to 256 exact colors", "[tiff-optimization]") {
  QTemporaryDir directory;
  REQUIRE(directory.isValid());
  QImage image(256, 1, QImage::Format_RGB32);
  for (int x = 0; x < image.width(); ++x) {
    image.setPixelColor(x, 0, QColor(x, (x + 1) & 0xFF, (x + 2) & 0xFF));
  }
  const QString path = writeSourceTiff(directory, QStringLiteral("256-colors.tif"), image);

  const auto result = TiffWriter::optimizeRenderedPdfPage(path, 300);
  CHECK(result.mode == TiffWriter::RenderedPageColorMode::IndexedColor);
  const TiffMetadata metadata = readMetadata(path);
  CHECK(metadata.bitsPerSample == 8);
  CHECK(metadata.samplesPerPixel == 1);
  CHECK(metadata.photometric == PHOTOMETRIC_PALETTE);
  CHECK(metadata.compression == COMPRESSION_LZW);
  checkPixelsMatch(image, path);
}

TEST_CASE("Rendered TIFF optimization leaves pages with more than 256 colors in RGB", "[tiff-optimization]") {
  QTemporaryDir directory;
  REQUIRE(directory.isValid());
  QImage image(257, 1, QImage::Format_RGB32);
  for (int x = 0; x < image.width(); ++x) {
    image.setPixelColor(x, 0, QColor(x & 0xFF, (x >> 8) & 0xFF, 0));
  }
  const QString path = writeSourceTiff(directory, QStringLiteral("257-colors.tif"), image);

  const auto result = TiffWriter::optimizeRenderedPdfPage(path, 300);
  CHECK(result.mode == TiffWriter::RenderedPageColorMode::FullColor);
  const TiffMetadata metadata = readMetadata(path);
  CHECK(metadata.bitsPerSample == 8);
  CHECK(metadata.samplesPerPixel == 3);
  CHECK(metadata.photometric == PHOTOMETRIC_RGB);
  checkPixelsMatch(image, path);
}

TEST_CASE("Rendered TIFF optimization leaves multi-directory TIFFs unchanged", "[tiff-optimization]") {
  QTemporaryDir directory;
  REQUIRE(directory.isValid());
  QImage image(2, 1, QImage::Format_RGB32);
  image.setPixelColor(0, 0, Qt::black);
  image.setPixelColor(1, 0, Qt::white);
  const QString path = writeSourceTiff(directory, QStringLiteral("multi-page.tif"), image);
  appendOnePixelRgbDirectory(path);

  QFile source(path);
  REQUIRE(source.open(QIODevice::ReadOnly));
  const QByteArray original = source.readAll();
  source.close();

  const auto result = TiffWriter::optimizeRenderedPdfPage(path, 300);

  CHECK(result.mode == TiffWriter::RenderedPageColorMode::Unsupported);
  REQUIRE(source.open(QIODevice::ReadOnly));
  CHECK(source.readAll() == original);
}
