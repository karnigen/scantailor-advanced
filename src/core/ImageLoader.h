// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#ifndef SCANTAILOR_CORE_IMAGELOADER_H_
#define SCANTAILOR_CORE_IMAGELOADER_H_

#include "ImageAllocationLimit.h"

class ImageId;
class QImage;
class QString;
class QIODevice;

class ImageLoader {
 public:
  static QImage load(const QString& filePath, int pageNum = 0);

  static QImage load(const QString& filePath, int pageNum, ImageAllocationLimitInfo* limitInfo);

  static QImage load(const ImageId& imageId);

  static QImage load(const ImageId& imageId, ImageAllocationLimitInfo* limitInfo);

  static QImage load(QIODevice& ioDev, int pageNum);

  static QImage load(QIODevice& ioDev, int pageNum, ImageAllocationLimitInfo* limitInfo);
};


#endif
