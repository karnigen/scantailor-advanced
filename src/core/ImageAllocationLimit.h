// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_CORE_IMAGEALLOCATIONLIMIT_H_
#define SCANTAILOR_CORE_IMAGEALLOCATIONLIMIT_H_

#include <QReadWriteLock>
#include <QSize>

namespace image_allocation_limit {
constexpr int kMaxLimitMb = 8 * 1024;
}

struct ImageAllocationLimitInfo {
  QSize imageSize;
  int currentLimitMb = 0;
  int requiredLimitMb = 0;
  bool exceeded = false;
  bool dimensionsKnown = false;

  bool canRaiseLimit() const {
    return exceeded && requiredLimitMb > 0 && requiredLimitMb <= image_allocation_limit::kMaxLimitMb;
  }
};

class ImageAllocationLimit {
 public:
  enum class UpdateResult { Applied, Busy, Invalid };

  static bool hasEnvironmentOverride();

  // Captures Qt's limit before a saved application limit is applied.
  static int defaultLimitMb();

  // Callers must hold a read lock across this check and the image decode.
  static bool exceeds(const QSize& imageSize, int bitsPerPixel, ImageAllocationLimitInfo* info);

  static UpdateResult setLimitMb(int limitMb, int* effectiveLimitMb);

  // Non-blocking; Busy means retry later, while Invalid means the value is outside the supported range.
  static UpdateResult trySetLimitMb(int limitMb, int* effectiveLimitMb);

  static QReadWriteLock& readWriteLock();
};

#endif  // SCANTAILOR_CORE_IMAGEALLOCATIONLIMIT_H_
