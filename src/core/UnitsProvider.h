// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#ifndef SCANTAILOR_CORE_UNITSPROVIDER_H_
#define SCANTAILOR_CORE_UNITSPROVIDER_H_

#include <foundation/NonCopyable.h>

#include <list>
#include <memory>

#include "UnitsListener.h"

class Dpi;

class UnitsProvider {
  DECLARE_NON_COPYABLE(UnitsProvider)
 private:
  UnitsProvider();

 public:
  static UnitsProvider& getInstance();

  Units getUnits() const { return m_units; }

  void setUnits(Units units);

  void addListener(UnitsListener* listener);

  void removeListener(UnitsListener* listener);

  void convertFrom(double& horizontalValue, double& verticalValue, Units fromUnits, const Dpi& dpi) const;

  void convertTo(double& horizontalValue, double& verticalValue, Units toUnits, const Dpi& dpi) const;

 protected:
  void unitsChanged();

 private:
  std::list<UnitsListener*> m_unitsListeners;
  Units m_units;
};


#endif  // SCANTAILOR_CORE_UNITSPROVIDER_H_
