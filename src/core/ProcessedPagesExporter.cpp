// SPDX-FileCopyrightText: 2026 ScanTailor Advanced contributors
// SPDX-License-Identifier: GPL-3.0-only

#include "ProcessedPagesExporter.h"

#include <QDir>
#include <QFileInfo>
#include <QImageWriter>
#include <QMarginsF>
#include <QPageLayout>
#include <QPageSize>
#include <QPainter>
#include <QPdfWriter>
#include <QSaveFile>

#include <webp/encode.h>
#include <webp/mux.h>

#include "ImageLoader.h"
#include "OutputFileNameGenerator.h"
#include "PageInfo.h"
#include "PageSequence.h"
#include "PathUtils.h"

namespace {

void setError(QString* errorMessage, const QString& message) {
  if (errorMessage) {
    *errorMessage = message;
  }
}

QImage loadProcessedPage(const PageInfo& page,
                         const OutputFileNameGenerator& fileNameGenerator,
                         QString* errorMessage) {
  const QString path = core::toQString(fileNameGenerator.filePathFor(page.id()));
  QImage image = ImageLoader::load(path, 0);
  if (image.isNull()) {
    setError(errorMessage,
             QStringLiteral("Could not read processed page: %1. Run batch processing through the Output stage first.")
                 .arg(path));
  }
  return image;
}

void appendLittleEndian16(QByteArray& data, const quint16 value) {
  data.append(static_cast<char>(value & 0xff));
  data.append(static_cast<char>((value >> 8) & 0xff));
}

void appendLittleEndian32(QByteArray& data, const quint32 value) {
  appendLittleEndian16(data, static_cast<quint16>(value & 0xffff));
  appendLittleEndian16(data, static_cast<quint16>((value >> 16) & 0xffff));
}

void appendExifEntry(QByteArray& data, const quint16 tag, const quint16 type, const quint32 value) {
  appendLittleEndian16(data, tag);
  appendLittleEndian16(data, type);
  appendLittleEndian32(data, 1);
  appendLittleEndian32(data, value);
}

QByteArray exifResolution(const QImage& image) {
  const auto dpiFromDpm = [](const int dpm) { return static_cast<quint32>(qMax(1, qRound(dpm * 0.0254))); };
  const int dpmX = image.dotsPerMeterX() > 0 ? image.dotsPerMeterX() : qRound(150.0 / 0.0254);
  const int dpmY = image.dotsPerMeterY() > 0 ? image.dotsPerMeterY() : qRound(150.0 / 0.0254);
  const quint32 xDpi = dpiFromDpm(dpmX);
  const quint32 yDpi = dpiFromDpm(dpmY);

  QByteArray exif("Exif\0\0", 6);
  appendLittleEndian16(exif, 0x4949);
  appendLittleEndian16(exif, 42);
  appendLittleEndian32(exif, 8);
  appendLittleEndian16(exif, 3);
  appendExifEntry(exif, 0x011a, 5, 50);
  appendExifEntry(exif, 0x011b, 5, 58);
  appendLittleEndian16(exif, 0x0128);
  appendLittleEndian16(exif, 3);
  appendLittleEndian32(exif, 1);
  appendLittleEndian16(exif, 2);
  appendLittleEndian16(exif, 0);
  appendLittleEndian32(exif, 0);
  appendLittleEndian32(exif, xDpi);
  appendLittleEndian32(exif, 1);
  appendLittleEndian32(exif, yDpi);
  appendLittleEndian32(exif, 1);
  return exif;
}

struct WebPBufferDeleter {
  void operator()(uint8_t* data) const { WebPFree(data); }
};

struct WebPMuxDeleter {
  void operator()(WebPMux* mux) const { WebPMuxDelete(mux); }
};

bool writeWebP(const QString& path, const QImage& image, QString* errorMessage) {
  const QImage rgba = image.convertToFormat(QImage::Format_RGBA8888);
  uint8_t* encoded = nullptr;
  const size_t encodedSize
      = rgba.isGrayscale()
            ? WebPEncodeLosslessRGBA(rgba.constBits(), rgba.width(), rgba.height(), rgba.bytesPerLine(), &encoded)
            : WebPEncodeRGBA(rgba.constBits(), rgba.width(), rgba.height(), rgba.bytesPerLine(), 90.0f, &encoded);
  std::unique_ptr<uint8_t, WebPBufferDeleter> encodedData(encoded);
  if (encodedSize == 0 || !encodedData) {
    setError(errorMessage, QStringLiteral("WebP encoding failed."));
    return false;
  }

  std::unique_ptr<WebPMux, WebPMuxDeleter> mux(WebPMuxNew());
  if (!mux) {
    setError(errorMessage, QStringLiteral("Could not allocate the WebP container."));
    return false;
  }

  WebPData imageData;
  WebPDataInit(&imageData);
  imageData.bytes = encodedData.get();
  imageData.size = encodedSize;
  if (WebPMuxSetImage(mux.get(), &imageData, 1) != WEBP_MUX_OK) {
    setError(errorMessage, QStringLiteral("Could not prepare the WebP image."));
    return false;
  }

  const QByteArray exif = exifResolution(image);
  WebPData exifData;
  WebPDataInit(&exifData);
  exifData.bytes = reinterpret_cast<const uint8_t*>(exif.constData());
  exifData.size = static_cast<size_t>(exif.size());
  if (WebPMuxSetChunk(mux.get(), "EXIF", &exifData, 1) != WEBP_MUX_OK) {
    setError(errorMessage, QStringLiteral("Could not add output resolution to the WebP image."));
    return false;
  }

  WebPData assembled;
  WebPDataInit(&assembled);
  if (WebPMuxAssemble(mux.get(), &assembled) != WEBP_MUX_OK) {
    setError(errorMessage, QStringLiteral("Could not assemble the WebP image."));
    return false;
  }

  QSaveFile file(path);
  if (!file.open(QIODevice::WriteOnly)) {
    WebPDataClear(&assembled);
    setError(errorMessage, file.errorString());
    return false;
  }
  const qint64 bytesToWrite = static_cast<qint64>(assembled.size);
  const qint64 bytesWritten = file.write(reinterpret_cast<const char*>(assembled.bytes), bytesToWrite);
  WebPDataClear(&assembled);
  if (bytesWritten != bytesToWrite) {
    file.cancelWriting();
    setError(errorMessage, file.errorString());
    return false;
  }
  if (!file.commit()) {
    setError(errorMessage, file.errorString());
    return false;
  }
  return true;
}

bool writeImage(const QString& path, const QString& format, const QImage& image, QString* errorMessage) {
  if (format == QStringLiteral("webp")) {
    return writeWebP(path, image, errorMessage);
  }

  QSaveFile file(path);
  if (!file.open(QIODevice::WriteOnly)) {
    setError(errorMessage, file.errorString());
    return false;
  }

  QImageWriter writer(&file, format.toLatin1());
  writer.setQuality(90);
  if (!writer.write(image)) {
    file.cancelWriting();
    setError(errorMessage, writer.errorString());
    return false;
  }
  if (!file.commit()) {
    setError(errorMessage, file.errorString());
    return false;
  }
  return true;
}

QPageLayout pageLayoutFor(const QImage& image) {
  const double fallbackDpm = 150.0 / 0.0254;
  const double dpmX = image.dotsPerMeterX() > 0 ? image.dotsPerMeterX() : fallbackDpm;
  const double dpmY = image.dotsPerMeterY() > 0 ? image.dotsPerMeterY() : fallbackDpm;
  const QSizeF sizeMm(image.width() * 1000.0 / dpmX, image.height() * 1000.0 / dpmY);
  const QPageSize pageSize(sizeMm, QPageSize::Millimeter, QString(), QPageSize::ExactMatch);
  const QPageLayout::Orientation orientation
      = sizeMm.width() > sizeMm.height() ? QPageLayout::Landscape : QPageLayout::Portrait;
  return QPageLayout(pageSize, orientation, QMarginsF(0.0, 0.0, 0.0, 0.0), QPageLayout::Millimeter);
}

}  // namespace

QStringList ProcessedPagesExporter::supportedImageFormats() {
  QStringList formats;
  formats.append(QStringLiteral("webp"));
  for (const QByteArray& format : QImageWriter::supportedImageFormats()) {
    const QString normalized = QString::fromLatin1(format).toLower();
    if (!formats.contains(normalized)) {
      formats.append(normalized);
    }
  }
  formats.sort(Qt::CaseInsensitive);
  formats.removeOne(QStringLiteral("webp"));
  formats.prepend(QStringLiteral("webp"));
  return formats;
}

bool ProcessedPagesExporter::exportImages(const PageSequence& pages,
                                          const OutputFileNameGenerator& fileNameGenerator,
                                          const QString& outputDirectory,
                                          const QString& format,
                                          const ProgressCallback& progress,
                                          QString* errorMessage) {
  const QString normalizedFormat = format.toLower();
  if (!supportedImageFormats().contains(normalizedFormat)) {
    setError(errorMessage, QStringLiteral("Unsupported output image format: %1").arg(format));
    return false;
  }
  if (pages.numPages() == 0) {
    setError(errorMessage, QStringLiteral("There are no pages to export."));
    return false;
  }
  if (!QDir().mkpath(outputDirectory)) {
    setError(errorMessage, QStringLiteral("Could not create output directory: %1").arg(outputDirectory));
    return false;
  }

  int completed = 0;
  const int total = static_cast<int>(pages.numPages());
  for (const PageInfo& page : pages) {
    QImage image = loadProcessedPage(page, fileNameGenerator, errorMessage);
    if (image.isNull()) {
      return false;
    }
    const QFileInfo sourceInfo(core::toQString(fileNameGenerator.filePathFor(page.id())));
    const QString outputPath
        = QDir(outputDirectory).filePath(sourceInfo.completeBaseName() + QLatin1Char('.') + normalizedFormat);
    if (!writeImage(outputPath, normalizedFormat, image, errorMessage)) {
      return false;
    }
    ++completed;
    if (progress && !progress(completed, total)) {
      setError(errorMessage, QStringLiteral("Export canceled after %1 of %2 pages.").arg(completed).arg(total));
      return false;
    }
  }
  return true;
}

bool ProcessedPagesExporter::exportPdf(const PageSequence& pages,
                                       const OutputFileNameGenerator& fileNameGenerator,
                                       const QString& outputFilePath,
                                       const ProgressCallback& progress,
                                       QString* errorMessage) {
  if (pages.numPages() == 0) {
    setError(errorMessage, QStringLiteral("There are no pages to export."));
    return false;
  }

  QSaveFile file(outputFilePath);
  if (!file.open(QIODevice::WriteOnly)) {
    setError(errorMessage, file.errorString());
    return false;
  }

  {
    QPdfWriter writer(&file);
    writer.setResolution(300);
    writer.setTitle(QFileInfo(outputFilePath).completeBaseName());
    QPainter painter(&writer);
    if (!painter.isActive()) {
      file.cancelWriting();
      setError(errorMessage, QStringLiteral("Could not start the PDF writer."));
      return false;
    }

    int completed = 0;
    const int total = static_cast<int>(pages.numPages());
    for (const PageInfo& page : pages) {
      QImage image = loadProcessedPage(page, fileNameGenerator, errorMessage);
      if (image.isNull()) {
        file.cancelWriting();
        return false;
      }

      const QPageLayout layout = pageLayoutFor(image);
      if (!writer.setPageLayout(layout)) {
        painter.end();
        file.cancelWriting();
        setError(errorMessage, QStringLiteral("Could not set the PDF page dimensions."));
        return false;
      }
      if (completed > 0 && !writer.newPage()) {
        painter.end();
        file.cancelWriting();
        setError(errorMessage, QStringLiteral("Could not create a PDF page."));
        return false;
      }
      painter.drawImage(writer.pageLayout().paintRectPixels(writer.resolution()), image);
      ++completed;
      if (progress && !progress(completed, total)) {
        painter.end();
        file.cancelWriting();
        setError(errorMessage, QStringLiteral("PDF export canceled after %1 of %2 pages.").arg(completed).arg(total));
        return false;
      }
    }

    if (!painter.end()) {
      file.cancelWriting();
      setError(errorMessage, QStringLiteral("Could not finish the PDF document."));
      return false;
    }
  }

  if (!file.commit()) {
    setError(errorMessage, file.errorString());
    return false;
  }
  return true;
}
