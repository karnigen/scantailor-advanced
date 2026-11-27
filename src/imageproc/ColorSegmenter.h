// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#ifndef SCANTAILOR_IMAGEPROC_COLORSEGMENTER_H_
#define SCANTAILOR_IMAGEPROC_COLORSEGMENTER_H_

#include <QImage>

#include "Dpi.h"

namespace imageproc {
class BinaryImage;
class GrayImage;

class ColorSegmenter {
 public:
  ColorSegmenter(const Dpi& dpi,
                 int noiseThreshold,
                 int redThresholdAdjustment,
                 int greenThresholdAdjustment,
                 int blueThresholdAdjustment);

  ColorSegmenter(const Dpi& dpi, int noiseThreshold);

  QImage segment(const BinaryImage& image, const QImage& colorImage) const;

  GrayImage segment(const BinaryImage& image, const GrayImage& grayImage) const;

 private:
  Dpi m_dpi;
  int m_noiseThreshold;
  int m_redThresholdAdjustment;
  int m_greenThresholdAdjustment;
  int m_blueThresholdAdjustment;
};
}  // namespace imageproc

#endif  // SCANTAILOR_IMAGEPROC_COLORSEGMENTER_H_
