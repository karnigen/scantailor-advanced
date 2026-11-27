// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#ifndef SCANTAILOR_PAGE_LAYOUT_THUMBNAIL_H_
#define SCANTAILOR_PAGE_LAYOUT_THUMBNAIL_H_

#include <QRectF>
#include <QTransform>
#include <memory>

#include "ImageTransformation.h"
#include "Params.h"
#include "ThumbnailBase.h"

class ThumbnailPixmapCache;
class ImageId;

namespace page_layout {
class Thumbnail : public ThumbnailBase {
 public:
  Thumbnail(std::shared_ptr<ThumbnailPixmapCache> thumbnailCache,
            const QSizeF& maxSize,
            const ImageId& imageId,
            const Params& params,
            const ImageTransformation& xform,
            const QPolygonF& physContentRect,
            bool deviant);

  void paintOverImage(QPainter& painter, const QTransform& imageToDisplay, const QTransform& thumbToDisplay) override;

 private:
  Params m_params;
  QRectF m_virtContentRect;
  QRectF m_virtOuterRect;
  bool m_deviant;
};
}  // namespace page_layout
#endif
