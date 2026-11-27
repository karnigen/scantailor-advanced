// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <QtGui/QTransform>

#include "CylindricalSurfaceDewarper.h"

class QRect;

namespace dewarping {
class DistortionModel;

class DewarpingPointMapper {
 public:
  DewarpingPointMapper(const dewarping::DistortionModel& distortionModel,
                       double depthPerception,
                       const QTransform& distortionModelToOutput,
                       const QRect& outputContentRect,
                       const QTransform& postTransform = QTransform());

  /**
   * Similar to CylindricalSurfaceDewarper::mapToDewarpedSpace(),
   * except it maps to dewarped image coordinates rather than
   * to normalized dewarped coordinates.
   */
  QPointF mapToDewarpedSpace(const QPointF& warpedPt) const;

  /**
   * Similar to CylindricalSurfaceDewarper::mapToWarpedSpace(),
   * except it maps from dewarped image coordinates rather than
   * from normalized dewarped coordinates.
   */
  QPointF mapToWarpedSpace(const QPointF& dewarpedPt) const;

 private:
  CylindricalSurfaceDewarper m_dewarper;
  double m_modelDomainLeft;
  double m_modelDomainTop;
  double m_modelXScaleFromNormalized;
  double m_modelYScaleFromNormalized;
  double m_modelXScaleToNormalized;
  double m_modelYScaleToNormalized;
  QTransform m_postTransform;
};
}  // namespace dewarping
