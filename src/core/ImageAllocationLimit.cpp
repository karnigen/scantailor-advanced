// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

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

bool ImageAllocationLimit::hasEnvironmentOverride() {
  bool valueOk = false;
  const int limitMb = qEnvironmentVariableIntValue("QT_IMAGEIO_MAXALLOC", &valueOk);
  return valueOk && limitMb >= 0;
}

int ImageAllocationLimit::defaultLimitMb() {
  static const int defaultLimit = QImageReader::allocationLimit();
  return defaultLimit;
}

bool ImageAllocationLimit::exceeds(const QSize& imageSize, const int bitsPerPixel, ImageAllocationLimitInfo* info) {
  ImageAllocationLimitInfo localInfo;
  if (!info) {
    info = &localInfo;
  }
  *info = ImageAllocationLimitInfo();
  info->dimensionsKnown = imageSize.width() > 0 && imageSize.height() > 0;
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
  info->requiredLimitMb = static_cast<int>(requiredLimitMb);
  info->exceeded = info->requiredLimitMb > currentLimitMb;
  return info->exceeded;
}

ImageAllocationLimit::UpdateResult ImageAllocationLimit::setLimitMb(const int limitMb, int* effectiveLimitMb) {
  if (limitMb < 0 || limitMb > image_allocation_limit::kMaxLimitMb) {
    return UpdateResult::Invalid;
  }

  QWriteLocker lock(&readWriteLock());
  QImageReader::setAllocationLimit(limitMb);
  if (effectiveLimitMb) {
    *effectiveLimitMb = QImageReader::allocationLimit();
  }
  return UpdateResult::Applied;
}

ImageAllocationLimit::UpdateResult ImageAllocationLimit::trySetLimitMb(const int limitMb, int* effectiveLimitMb) {
  if (limitMb < 0 || limitMb > image_allocation_limit::kMaxLimitMb) {
    return UpdateResult::Invalid;
  }

  QReadWriteLock& lock = readWriteLock();
  if (!lock.tryLockForWrite()) {
    return UpdateResult::Busy;
  }
  QImageReader::setAllocationLimit(limitMb);
  if (effectiveLimitMb) {
    *effectiveLimitMb = QImageReader::allocationLimit();
  }
  lock.unlock();
  return UpdateResult::Applied;
}
