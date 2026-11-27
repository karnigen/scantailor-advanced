// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#ifndef SCANTAILOR_DESKEW_THUMBNAIL_H_
#define SCANTAILOR_DESKEW_THUMBNAIL_H_

#include <memory>

#include "ThumbnailBase.h"

class QSizeF;
class ThumbnailPixmapCache;
class ImageId;
class ImageTransformation;

namespace deskew {
class Thumbnail : public ThumbnailBase {
 public:
  Thumbnail(std::shared_ptr<ThumbnailPixmapCache> thumbnailCache,
            const QSizeF& maxSize,
            const ImageId& imageId,
            const ImageTransformation& xform,
            bool deviant = false);

  void prePaintOverImage(QPainter& painter,
                         const QTransform& imageToDisplay,
                         const QTransform& thumbToDisplay) override;

 private:
  bool m_deviant;
};
}  // namespace deskew
#endif
