// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "ImageAllocationLimit.h"

#include <QImageReader>

#include <QReadLocker>
#include <QReadWriteLock>
#include <QWriteLocker>
#include <cstdint>
#include <limits>

QReadWriteLock& ImageAllocationLimit::readWriteLock() {
  static QReadWriteLock lock;
  return lock;
}

bool ImageAllocationLimit::exceeds(const QSize& imageSize, const int bitsPerPixel, ImageAllocationLimitInfo* info) {
  if (!info) {
    return false;
  }
  *info = ImageAllocationLimitInfo();
  if (imageSize.width() <= 0 || imageSize.height() <= 0 || bitsPerPixel <= 0) {
    return false;
  }

  const int currentLimitMb = QImageReader::allocationLimit();
  if (currentLimitMb <= 0) {
    return false;
  }

  constexpr std::uint64_t kBytesPerMiB = 1024 * 1024;
  const std::uint64_t maxBytes = static_cast<std::uint64_t>(std::numeric_limits<int>::max()) * kBytesPerMiB;
  const std::uint64_t width = static_cast<std::uint64_t>(imageSize.width());
  const std::uint64_t height = static_cast<std::uint64_t>(imageSize.height());
  const std::uint64_t rowBits = width * static_cast<std::uint64_t>(bitsPerPixel);
  const std::uint64_t rowBytes = (rowBits + 7) / 8;

  info->imageSize = imageSize;
  info->currentLimitMb = currentLimitMb;
  info->exceeded = true;
  if (rowBytes > maxBytes / height) {
    return true;
  }

  const std::uint64_t requiredBytes = rowBytes * height;
  const std::uint64_t requiredLimitMb = (requiredBytes + kBytesPerMiB - 1) / kBytesPerMiB;
  if (requiredLimitMb > static_cast<std::uint64_t>(std::numeric_limits<int>::max())) {
    return true;
  }

  info->requiredLimitMb = static_cast<int>(requiredLimitMb);
  info->exceeded = info->requiredLimitMb > currentLimitMb;
  return info->exceeded;
}

void ImageAllocationLimit::setLimitMb(const int limitMb) {
  QWriteLocker lock(&readWriteLock());
  QImageReader::setAllocationLimit(limitMb);
}
