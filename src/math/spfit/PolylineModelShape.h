// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <QPointF>
#include <vector>

#include "FlagOps.h"
#include "ModelShape.h"
#include "NonCopyable.h"
#include "SqDistApproximant.h"
#include "VecNT.h"
#include "XSpline.h"

namespace spfit {
class PolylineModelShape : public ModelShape {
  DECLARE_NON_COPYABLE(PolylineModelShape)

 public:
  enum Flags { DEFAULT_FLAGS = 0, POLYLINE_FRONT = 1 << 0, POLYLINE_BACK = 1 << 1 };

  explicit PolylineModelShape(const std::vector<QPointF>& polyline);

  SqDistApproximant localSqDistApproximant(const QPointF& pt, FittableSpline::SampleFlags sampleFlags) const override;

 protected:
  virtual SqDistApproximant calcApproximant(const QPointF& pt,
                                            FittableSpline::SampleFlags sampleFlags,
                                            Flags polylineFlags,
                                            const FrenetFrame& frenetFrame,
                                            double signedCurvature) const;

 private:
  std::vector<XSpline::PointAndDerivs> m_vertices;
};


DEFINE_FLAG_OPS(PolylineModelShape::Flags)
}  // namespace spfit
