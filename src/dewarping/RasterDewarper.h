// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#ifndef SCANTAILOR_DEWARPING_RASTERDEWARPER_H_
#define SCANTAILOR_DEWARPING_RASTERDEWARPER_H_

class QImage;
class QSize;
class QRectF;
class QColor;

namespace dewarping {
class CylindricalSurfaceDewarper;

class RasterDewarper {
 public:
  static QImage dewarp(const QImage& src,
                       const QSize& dstSize,
                       const CylindricalSurfaceDewarper& distortionModel,
                       const QRectF& modelDomain,
                       const QColor& backgroundColor);
};
}  // namespace dewarping
#endif
