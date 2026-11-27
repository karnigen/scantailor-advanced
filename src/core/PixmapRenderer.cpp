// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#include "PixmapRenderer.h"

#include <QPainter>
#include <QPixmap>
#include <QRect>
#include <QRectF>
#include <QTransform>

void PixmapRenderer::drawPixmap(QPainter& painter, const QPixmap& pixmap) {
  const QTransform invTransform(painter.worldTransform().inverted());
  const QRectF srcRect(invTransform.map(QRectF(painter.viewport())).boundingRect());
  const QRectF boundedSrcRect(srcRect.intersected(pixmap.rect()));
  painter.drawPixmap(boundedSrcRect, pixmap, boundedSrcRect);
}
