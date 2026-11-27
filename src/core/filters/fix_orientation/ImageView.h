// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include "DragHandler.h"
#include "ImageTransformation.h"
#include "ImageTrim.h"
#include "ImageViewBase.h"
#include "OrthogonalRotation.h"
#include "ZoomHandler.h"

class QPaintEvent;

namespace fix_orientation {
class ImageView : public ImageViewBase {
  Q_OBJECT
 public:
  ImageView(const QImage& image,
            const QImage& downscaledImage,
            const ImageTransformation& xform,
            const ImageTrim& trim);

  ~ImageView() override;

 protected:
  void paintEvent(QPaintEvent* event) override;

 public slots:

  void setPreRotation(OrthogonalRotation rotation);

  void setTrim(ImageTrim trim);

 private:
  void updateImagePresentation();

  DragHandler m_dragHandler;
  ZoomHandler m_zoomHandler;
  ImageTransformation m_xform;
  ImageTrim m_trim;
  QPolygonF m_fullImageArea;
};
}  // namespace fix_orientation
