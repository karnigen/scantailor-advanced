// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "ImageLoader.h"

#include <QFile>
#include <QFileInfo>
#include <QImage>
#include <QReadLocker>
#include <QtGui/QImageReader>

#include "ImageId.h"
#include "TiffReader.h"

namespace {

constexpr int kConservativeUnknownFormatBitsPerPixel = 128;

}  // namespace

QImage ImageLoader::load(const ImageId& imageId) {
  return load(imageId.filePath(), imageId.zeroBasedPage());
}

QImage ImageLoader::load(const ImageId& imageId, ImageAllocationLimitInfo* limitInfo) {
  return load(imageId.filePath(), imageId.zeroBasedPage(), limitInfo);
}

QImage ImageLoader::load(const QString& filePath, const int pageNum) {
  return load(filePath, pageNum, nullptr);
}

QImage ImageLoader::load(const QString& filePath, const int pageNum, ImageAllocationLimitInfo* limitInfo) {
  if (limitInfo) {
    *limitInfo = ImageAllocationLimitInfo();
  }
  QFile file(filePath);
  if (!file.open(QIODevice::ReadOnly)) {
    return QImage();
  }
  return load(file, pageNum, limitInfo);
}

QImage ImageLoader::load(QIODevice& ioDev, const int pageNum) {
  return load(ioDev, pageNum, nullptr);
}

QImage ImageLoader::load(QIODevice& ioDev, const int pageNum, ImageAllocationLimitInfo* limitInfo) {
  ImageAllocationLimitInfo localLimitInfo;
  if (!limitInfo) {
    limitInfo = &localLimitInfo;
  }
  *limitInfo = ImageAllocationLimitInfo();
  if (TiffReader::canRead(ioDev)) {
    return TiffReader::readImage(ioDev, pageNum, limitInfo);
  }

  if (pageNum != 0) {
    // Qt can only load the first page of multi-page images.
    return QImage();
  }

  QReadLocker allocationLimitLock(&ImageAllocationLimit::readWriteLock());
  QImage image;
  QImageReader reader(&ioDev);
  reader.setAutoTransform(true);  // Helps with orientation and some formats (issue #20).
  // Issue #98: on some builds Qt may not detect JPG format; force it for .jpg/.jpeg files.
  if (QFile* file = qobject_cast<QFile*>(&ioDev)) {
    const QString ext = QFileInfo(file->fileName()).suffix().toLower();
    if (ext == QStringLiteral("jpg") || ext == QStringLiteral("jpeg")) {
      reader.setFormat("jpeg");
    }
  }
  const QImage::Format format = reader.imageFormat();
  const int bitsPerPixel = format == QImage::Format_Invalid
                               ? kConservativeUnknownFormatBitsPerPixel
                               : qMax(32, static_cast<int>(QImage::toPixelFormat(format).bitsPerPixel()));
  if (ImageAllocationLimit::exceeds(reader.size(), bitsPerPixel, limitInfo)) {
    return QImage();
  }
  reader.read(&image);
  return image;
}
