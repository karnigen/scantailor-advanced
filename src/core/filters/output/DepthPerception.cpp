// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#include "DepthPerception.h"

#include <foundation/Utils.h>

using namespace foundation;

namespace output {
DepthPerception::DepthPerception() : m_value(defaultValue()) {}

DepthPerception::DepthPerception(double value) : m_value(qBound(minValue(), value, maxValue())) {}

DepthPerception::DepthPerception(const QString& fromString) {
  bool ok = false;
  m_value = fromString.toDouble(&ok);
  if (!ok) {
    m_value = defaultValue();
  } else {
    m_value = qBound(minValue(), m_value, maxValue());
  }
}

QString DepthPerception::toString() const {
  return Utils::doubleToString(m_value);
}

void DepthPerception::setValue(double value) {
  m_value = qBound(minValue(), value, maxValue());
}
}  // namespace output