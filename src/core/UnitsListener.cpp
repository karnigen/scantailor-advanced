// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#include "UnitsListener.h"

#include "UnitsProvider.h"

UnitsListener::UnitsListener() {
  UnitsProvider::getInstance().addListener(this);
}

UnitsListener::~UnitsListener() {
  UnitsProvider::getInstance().removeListener(this);
}