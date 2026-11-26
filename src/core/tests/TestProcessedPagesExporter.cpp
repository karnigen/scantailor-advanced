// Copyright (C) 2026  ScanTailor Advanced contributors
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

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
  CHECK(webp.contains(QByteArrayLiteral("EXIF")));
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
