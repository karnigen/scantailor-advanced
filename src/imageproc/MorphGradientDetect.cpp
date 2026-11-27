// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#include "MorphGradientDetect.h"

#include "GrayRasterOp.h"
#include "Grayscale.h"
#include "Morphology.h"

namespace imageproc {
GrayImage morphGradientDetectDarkSide(const GrayImage& image, const QSize& area) {
  GrayImage lighter(erodeGray(image, area, 0x00));
  grayRasterOp<GRopUnclippedSubtract<GRopDst, GRopSrc>>(lighter, image);
  return lighter;
}

GrayImage morphGradientDetectLightSide(const GrayImage& image, const QSize& area) {
  GrayImage darker(dilateGray(image, area, 0xff));
  grayRasterOp<GRopUnclippedSubtract<GRopSrc, GRopDst>>(darker, image);
  return darker;
}
}  // namespace imageproc
