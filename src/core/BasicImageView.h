// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#ifndef SCANTAILOR_CORE_BASICIMAGEVIEW_H_
#define SCANTAILOR_CORE_BASICIMAGEVIEW_H_

#include <QImage>

#include "DragHandler.h"
#include "ImagePixmapUnion.h"
#include "ImageViewBase.h"
#include "Margins.h"
#include "ZoomHandler.h"

class BasicImageView : public ImageViewBase {
  Q_OBJECT
 public:
  explicit BasicImageView(const QImage& image,
                          const ImagePixmapUnion& downscaledImage = ImagePixmapUnion(),
                          const Margins& margins = Margins());

  ~BasicImageView() override;

 private:
  DragHandler m_dragHandler;
  ZoomHandler m_zoomHandler;
};


#endif
