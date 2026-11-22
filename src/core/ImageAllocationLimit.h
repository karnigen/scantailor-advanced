// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_CORE_IMAGEALLOCATIONLIMIT_H_
#define SCANTAILOR_CORE_IMAGEALLOCATIONLIMIT_H_

#include <QReadWriteLock>
#include <QSize>

struct ImageAllocationLimitInfo {
  QSize imageSize;
  int currentLimitMb = 0;
  int requiredLimitMb = 0;
  bool exceeded = false;

  bool canRaiseLimit() const { return exceeded && requiredLimitMb > 0; }
};

class ImageAllocationLimit {
 public:
  static bool exceeds(const QSize& imageSize, int bitsPerPixel, ImageAllocationLimitInfo* info);

  static int setLimitMb(int limitMb);

  static QReadWriteLock& readWriteLock();
};

#endif  // SCANTAILOR_CORE_IMAGEALLOCATIONLIMIT_H_
