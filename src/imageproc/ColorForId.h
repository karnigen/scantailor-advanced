// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#ifndef SCANTAILOR_IMAGEPROC_COLORFORID_H_
#define SCANTAILOR_IMAGEPROC_COLORFORID_H_

#include <QColor>

#include "BitOps.h"

namespace imageproc {
/**
 * \brief Generates a color corresponding to a particular numeric ID.
 *
 * Colors for IDs that are numerically close will tend to be significantly
 * different.  Positive IDs are handled better.
 */
template <typename T>
QColor colorForId(T id) {
  const int bitsUnused = countMostSignificantZeroes(id);
  const int bitsUsed = sizeof(T) * 8 - bitsUnused;
  const T reversed = reverseBits(id) >> bitsUnused;
  const T mask = (T(1) << bitsUsed) - 1;

  const double H = 0.99 * double(reversed + 1) / (mask + 1);
  const double S = 1.0;
  const double V = 1.0;
  QColor color;
  color.setHsvF(H, S, V);
  return color;
}
}  // namespace imageproc
#endif
