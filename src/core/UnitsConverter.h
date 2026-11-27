// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#ifndef SCANTAILOR_CORE_UNITSCONVERTER_H_
#define SCANTAILOR_CORE_UNITSCONVERTER_H_


#include <QtGui/QTransform>

#include "Dpi.h"
#include "Units.h"

class UnitsConverter {
 public:
  UnitsConverter() = default;

  explicit UnitsConverter(const Dpi& dpi);

  void convert(double& horizontalValue, double& verticalValue, Units fromUnits, Units toUnits) const;

  QTransform transform(Units fromUnits, Units toUnits) const;

  const Dpi& getDpi() const { return m_dpi; }

  void setDpi(const Dpi& dpi) { m_dpi = dpi; }

 private:
  Dpi m_dpi;
};


#endif  // SCANTAILOR_CORE_UNITSCONVERTER_H_
