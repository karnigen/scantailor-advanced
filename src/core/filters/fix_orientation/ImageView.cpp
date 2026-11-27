// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#include "ImageView.h"

#include <QPaintEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPen>

#include "ImagePresentation.h"

namespace fix_orientation {
ImageView::ImageView(const QImage& image,
                     const QImage& downscaledImage,
                     const ImageTransformation& xform,
                     const ImageTrim& trim)
    : ImageViewBase(image,
                    downscaledImage,
                    ImagePresentation(xform.transform(), xform.transform().map(QPolygonF(xform.origRect())))),
      m_dragHandler(*this),
      m_zoomHandler(*this),
      m_xform(xform),
      m_trim(trim),
      m_fullImageArea(xform.transform().map(QPolygonF(xform.origRect()))) {
  rootInteractionHandler().makeLastFollower(m_dragHandler);
  rootInteractionHandler().makeLastFollower(m_zoomHandler);
}

ImageView::~ImageView() = default;

void ImageView::paintEvent(QPaintEvent* event) {
  ImageViewBase::paintEvent(event);

  QPainter painter(viewport());
  painter.setRenderHint(QPainter::Antialiasing);
  painter.setWorldTransform(virtualToWidget());

  QPainterPath fullImagePath;
  fullImagePath.addPolygon(m_fullImageArea);
  QPainterPath cropPath;
  cropPath.addPolygon(m_xform.resultingPreCropArea());
  const QPainterPath trimmedArea = fullImagePath.subtracted(cropPath);
  if (trimmedArea.isEmpty()) {
    return;
  }

  painter.fillPath(trimmedArea, QColor(220, 40, 40, 90));
  QPen cropPen(QColor(255, 255, 255, 230));
  cropPen.setWidthF(0.0);
  painter.setPen(cropPen);
  painter.setBrush(Qt::NoBrush);
  painter.drawPath(cropPath);
}

void ImageView::setPreRotation(const OrthogonalRotation rotation) {
  if (m_xform.preRotation() == rotation) {
    return;
  }

  m_xform.setPreRotation(rotation);
  setTrim(m_trim);
}

void ImageView::setTrim(const ImageTrim trim) {
  m_trim = trim;
  m_xform.setPreCropArea(QPolygonF());
  if (m_trim.enabled) {
    const QRect inner(m_trim.toInnerRect(m_xform.origRect().size().toSize()));
    m_xform.setPreCropArea(m_xform.origRectToPreCropSpace(QRectF(inner)));
  }
  updateImagePresentation();
}

void ImageView::updateImagePresentation() {
  m_fullImageArea = m_xform.transform().map(QPolygonF(m_xform.origRect()));
  updateTransform(ImagePresentation(m_xform.transform(), m_fullImageArea));
}
}  // namespace fix_orientation
