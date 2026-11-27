// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <Qt>

#include "BWColor.h"

class QPolygonF;
class QRectF;
class QImage;

namespace imageproc {
class BinaryImage;

class PolygonRasterizer {
 public:
  static void fill(BinaryImage& image, BWColor color, const QPolygonF& poly, Qt::FillRule fillRule);

  static void fillExcept(BinaryImage& image, BWColor color, const QPolygonF& poly, Qt::FillRule fillRule);

  static void grayFill(QImage& image, unsigned char color, const QPolygonF& poly, Qt::FillRule fillRule);

  static void grayFillExcept(QImage& image, unsigned char color, const QPolygonF& poly, Qt::FillRule fillRule);

 private:
  class Edge;
  class EdgeComponent;
  class EdgeOrderY;
  class EdgeOrderX;

  class Rasterizer;
};
}  // namespace imageproc
