// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#include "ImageView.h"

#include <QContextMenuEvent>
#include <QMenu>
#include <QPainter>

#include "ApplicationSettings.h"
#include "ImagePresentation.h"
#include "OutputMargins.h"

namespace output {
ImageView::ImageView(const QImage& image, const QImage& downscaledImage)
    : ImageViewBase(image, downscaledImage, ImagePresentation(QTransform(), QRectF(image.rect())), OutputMargins()),
      m_dragHandler(*this),
      m_zoomHandler(*this) {
  rootInteractionHandler().makeLastFollower(m_dragHandler);
  rootInteractionHandler().makeLastFollower(m_zoomHandler);
  setContextMenuPolicy(Qt::DefaultContextMenu);
}

ImageView::~ImageView() = default;

void ImageView::paintEvent(QPaintEvent* event) {
  ImageViewBase::paintEvent(event);
  if (ApplicationSettings::getInstance().isOutputShowGuidesEnabled()) {
    QPainter painter(viewport());
    painter.save();
    drawGuides(painter);
    painter.restore();
  }
}

void ImageView::drawGuides(QPainter& painter) {
  const QRect vp = viewport()->rect();
  const int cx = vp.center().x();
  const int cy = vp.center().y();
  painter.setPen(QPen(QColor(255, 0, 0, 120), 1.0, Qt::DashLine));
  painter.drawLine(cx, vp.top(), cx, vp.bottom());
  painter.drawLine(vp.left(), cy, vp.right(), cy);
}

void ImageView::contextMenuEvent(QContextMenuEvent* event) {
  QMenu menu(this);
  menu.setToolTipsVisible(true);
  QAction* showGuidesAction = menu.addAction(tr("Show guides"));
  showGuidesAction->setToolTip(tr("Show or hide center guides in the output preview."));
  showGuidesAction->setCheckable(true);
  showGuidesAction->setChecked(ApplicationSettings::getInstance().isOutputShowGuidesEnabled());
  connect(showGuidesAction, &QAction::toggled,
          [](bool checked) { ApplicationSettings::getInstance().setOutputShowGuidesEnabled(checked); });
  menu.exec(event->globalPos());
  update();
}
}  // namespace output
