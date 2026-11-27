// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <QColor>

#include "DragHandler.h"
#include "ImageViewBase.h"
#include "ZoomHandler.h"

#include <QPaintEvent>
#include <QPainter>

class ImageTransformation;

namespace output {
class ImageView : public ImageViewBase {
  Q_OBJECT
 public:
  ImageView(const QImage& image, const QImage& downscaledImage);

  ~ImageView() override;

 protected:
  void paintEvent(QPaintEvent* event) override;

  void contextMenuEvent(QContextMenuEvent* event) override;

 private:
  void drawGuides(QPainter& painter);

  DragHandler m_dragHandler;
  ZoomHandler m_zoomHandler;
};
}  // namespace output
