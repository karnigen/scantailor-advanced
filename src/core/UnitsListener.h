// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#pragma once


#include "Units.h"

class Dpi;

class UnitsListener {
 protected:
  UnitsListener();

 public:
  virtual ~UnitsListener();

  virtual void onUnitsChanged(Units units) = 0;
};
