// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <QRectF>
#include <QSizeF>
#include <QTransform>

class ImageTransformation;

namespace select_content {
class PhysSizeCalc {
  // Member-wise copying is OK.
 public:
  PhysSizeCalc();

  explicit PhysSizeCalc(const ImageTransformation& xform);

  QSizeF sizeMM(const QRectF& rectPx) const;

 private:
  QTransform m_virtToPhys;
};
}  // namespace select_content
