// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <QPainterPath>
#include <memory>

#include "ThumbnailBase.h"

class ThumbnailPixmapCache;
class QSizeF;
class QRectF;

/**
 * \brief A thumbnail represeting a page not completely processed.
 *
 * Suppose you have switched to the Deskew filter without running all images
 * through the previous filters.  The thumbnail view of the Deskew filter
 * is supposed to show already split and deskewed pages.  However, because
 * the previous filters were not applied to all pages, we can't show them
 * split and deskewed.  In that case, we show an image the best we can
 * (maybe split but not deskewed, maybe unsplit).  Also we draw a translucent
 * question mark over that image to indicate it's not shown the way it should.
 * This class implements drawing of such thumbnails with question marks.
 */
class IncompleteThumbnail : public ThumbnailBase {
 public:
  IncompleteThumbnail(std::shared_ptr<ThumbnailPixmapCache> thumbnailCache,
                      const QSizeF& maxSize,
                      const ImageId& imageId,
                      const ImageTransformation& imageXform);

  ~IncompleteThumbnail() override;

  static void drawQuestionMark(QPainter& painter, const QRectF& boundingRect);

 protected:
  void paintOverImage(QPainter& painter, const QTransform& imageToDisplay, const QTransform& thumbToDisplay) override;

 private:
  static QPainterPath m_sCachedPath;
};
