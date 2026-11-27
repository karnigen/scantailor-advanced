// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#include "FrenetFrame.h"

#include <cmath>

namespace spfit {
FrenetFrame::FrenetFrame(const Vec2d& origin, const Vec2d& tangentVector, YAxisDirection ydir) : m_origin(origin) {
  const double sqlen = tangentVector.squaredNorm();
  if (sqlen > 1e-6) {
    m_unitTangent = tangentVector / std::sqrt(sqlen);
    if (ydir == Y_POINTS_UP) {
      m_unitNormal[0] = -m_unitTangent[1];
      m_unitNormal[1] = m_unitTangent[0];
    } else {
      m_unitNormal[0] = m_unitTangent[1];
      m_unitNormal[1] = -m_unitTangent[0];
    }
  }
}
}  // namespace spfit
