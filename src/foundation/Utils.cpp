// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#include "Utils.h"

namespace foundation {
QString Utils::doubleToString(double val) {
  return QString::number(val, 'g', 16);
}
}  // namespace foundation