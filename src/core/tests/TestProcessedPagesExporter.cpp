// SPDX-FileCopyrightText: 2026 ScanTailor Advanced contributors
// SPDX-License-Identifier: GPL-3.0-only

#include <catch2/catch_test_macros.hpp>

#include <QDir>
#include <QFile>
#include <QImage>
#include <QTemporaryDir>

#include <DefaultParams.h>
#include <Dpi.h>
#include <FileNameDisambiguator.h>
#include <ImageMetadata.h>
#include <OutputFileNameGenerator.h>
#include <PageInfo.h>
#include <PageSequence.h>
#include <PathUtils.h>
#include <ProcessedPagesExporter.h>
#include <TiffWriter.h>
#include <filters/output/Params.h>

#include <memory>

namespace {

struct ExportFixture {
  ExportFixture()
      : tempDir(),
        outputDirectory(QDir(tempDir.path()).filePath(QStringLiteral("processed"))),
        generator(std::make_shared<FileNameDisambiguator>(), core::toFileSystemPath(outputDirectory), Qt::LeftToRight) {
    REQUIRE(tempDir.isValid());
    REQUIRE(QDir().mkpath(outputDirectory));
  }

  PageInfo addPage(const QString& sourcePath, const int color) {
    const QImage image(QSize(20, 30), QImage::Format_RGB32);
    QImage processedImage(image);
    processedImage.fill(color);
    processedImage.setDotsPerMeterX(qRound(150.0 / 0.0254));
    processedImage.setDotsPerMeterY(qRound(150.0 / 0.0254));

    const PageInfo page(PageId(ImageId(core::toFileSystemPath(sourcePath)), PageId::SINGLE_PAGE),
                        ImageMetadata(processedImage.size(), Dpi(150, 150)), 1, false, false);
    REQUIRE(TiffWriter::writeImage(core::toQString(generator.filePathFor(page.id())), processedImage));
    pages.append(page);
    return page;
  }

  QTemporaryDir tempDir;
  QString outputDirectory;
  OutputFileNameGenerator generator;
  PageSequence pages;
};

}  // namespace

TEST_CASE("Processed pages can be exported to WebP with resolution metadata", "[processed-pages-export]") {
  ExportFixture fixture;
  fixture.addPage(QStringLiteral("/input/page1.tif"), qRgb(40, 100, 160));
  const QString exportDirectory = QDir(fixture.tempDir.path()).filePath(QStringLiteral("webp"));
  QString errorMessage;

  REQUIRE(ProcessedPagesExporter::supportedImageFormats().contains(QStringLiteral("webp")));
  REQUIRE(ProcessedPagesExporter::exportImages(fixture.pages, fixture.generator, exportDirectory,
                                               QStringLiteral("webp"), {}, &errorMessage));

  QFile exported(QDir(exportDirectory).filePath(QStringLiteral("page1.webp")));
  REQUIRE(exported.open(QIODevice::ReadOnly));
  const QByteArray webp = exported.readAll();
  REQUIRE(webp.size() > 12);
  CHECK(webp.left(4) == QByteArrayLiteral("RIFF"));
  CHECK(webp.mid(8, 4) == QByteArrayLiteral("WEBP"));

  const auto readLittleEndian16 = [&webp](const qsizetype offset) {
    return static_cast<quint16>(static_cast<quint8>(webp.at(offset)))
           | static_cast<quint16>(static_cast<quint8>(webp.at(offset + 1)) << 8);
  };
  const auto readLittleEndian32 = [&webp](const qsizetype offset) {
    return static_cast<quint32>(static_cast<quint8>(webp.at(offset)))
           | (static_cast<quint32>(static_cast<quint8>(webp.at(offset + 1))) << 8)
           | (static_cast<quint32>(static_cast<quint8>(webp.at(offset + 2))) << 16)
           | (static_cast<quint32>(static_cast<quint8>(webp.at(offset + 3))) << 24);
  };

  qsizetype chunkOffset = 12;
  qsizetype exifOffset = -1;
  quint32 exifSize = 0;
  while (chunkOffset + 8 <= webp.size()) {
    const quint32 chunkSize = readLittleEndian32(chunkOffset + 4);
    REQUIRE(chunkSize <= static_cast<quint32>(webp.size() - chunkOffset - 8));
    if (webp.mid(chunkOffset, 4) == QByteArrayLiteral("EXIF")) {
      exifOffset = chunkOffset + 8;
      exifSize = chunkSize;
      break;
    }
    chunkOffset += 8 + chunkSize + (chunkSize & 1);
  }

  REQUIRE(exifOffset >= 0);
  REQUIRE(exifSize >= 66);
  CHECK(webp.mid(exifOffset, 6) == QByteArray("Exif\0\0", 6));
  const qsizetype tiffOffset = exifOffset + 6;
  CHECK(webp.mid(tiffOffset, 2) == QByteArrayLiteral("II"));
  CHECK(readLittleEndian16(tiffOffset + 2) == 42);

  const quint32 ifdOffset = readLittleEndian32(tiffOffset + 4);
  REQUIRE(ifdOffset + 2 + 3 * 12 + 4 <= exifSize - 6);
  const qsizetype ifd = tiffOffset + ifdOffset;
  REQUIRE(readLittleEndian16(ifd) == 3);

  const qsizetype xResolutionEntry = ifd + 2;
  CHECK(readLittleEndian16(xResolutionEntry) == 0x011a);
  CHECK(readLittleEndian16(xResolutionEntry + 2) == 5);
  CHECK(readLittleEndian32(xResolutionEntry + 4) == 1);
  const quint32 xResolutionOffset = readLittleEndian32(xResolutionEntry + 8);
  CHECK(readLittleEndian32(tiffOffset + xResolutionOffset) == 150);
  CHECK(readLittleEndian32(tiffOffset + xResolutionOffset + 4) == 1);

  const qsizetype yResolutionEntry = xResolutionEntry + 12;
  CHECK(readLittleEndian16(yResolutionEntry) == 0x011b);
  CHECK(readLittleEndian16(yResolutionEntry + 2) == 5);
  CHECK(readLittleEndian32(yResolutionEntry + 4) == 1);
  const quint32 yResolutionOffset = readLittleEndian32(yResolutionEntry + 8);
  CHECK(readLittleEndian32(tiffOffset + yResolutionOffset) == 150);
  CHECK(readLittleEndian32(tiffOffset + yResolutionOffset + 4) == 1);
}

TEST_CASE("Processed pages can be exported as a multi-page PDF", "[processed-pages-export]") {
  ExportFixture fixture;
  fixture.addPage(QStringLiteral("/input/page1.tif"), qRgb(40, 100, 160));
  fixture.addPage(QStringLiteral("/input/page2.tif"), qRgb(160, 100, 40));
  const QString pdfPath = QDir(fixture.tempDir.path()).filePath(QStringLiteral("pages.pdf"));
  QString errorMessage;

  REQUIRE(ProcessedPagesExporter::exportPdf(fixture.pages, fixture.generator, pdfPath, {}, &errorMessage));

  QFile exported(pdfPath);
  REQUIRE(exported.open(QIODevice::ReadOnly));
  const QByteArray pdf = exported.readAll();
  CHECK(pdf.left(5) == QByteArrayLiteral("%PDF-"));
  CHECK(exported.size() > 500);
}

TEST_CASE("New output pages default to a smaller resolution", "[processed-pages-export]") {
  CHECK(output::Params().outputDpi() == Dpi(150, 150));
  CHECK(DefaultParams::OutputParams().getDpi() == Dpi(150, 150));
}
