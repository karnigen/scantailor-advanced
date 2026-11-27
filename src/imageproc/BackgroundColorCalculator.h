// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <cstdint>

class QImage;
class QColor;
class QPolygonF;

namespace imageproc {
class GrayscaleHistogram;
class BinaryImage;

class BackgroundColorCalculator {
 public:
  static QColor calcDominantBackgroundColor(const QImage& img);

  static QColor calcDominantBackgroundColor(const QImage& img, const BinaryImage& mask);

  static QColor calcDominantBackgroundColor(const QImage& img, const QPolygonF& cropArea);

 private:
  static uint8_t calcDominantLevel(const int* hist);

  static QColor calcDominantColor(const QImage& img, const BinaryImage& backgroundMask);
};
}  // namespace imageproc
