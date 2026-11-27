// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#include "DewarpingOptions.h"

#include <foundation/Utils.h>

#include <cassert>

using namespace foundation;

namespace output {
DewarpingOptions::DewarpingOptions(DewarpingMode mode, bool needPostDeskew)
    : m_mode(mode), m_needPostDeskew(needPostDeskew), m_postDeskewAngle(0.0) {}

DewarpingOptions::DewarpingOptions(const QDomElement& el)
    : m_mode(parseDewarpingMode(el.attribute("mode"))),
      m_needPostDeskew(el.attribute("postDeskew", "1") == "1"),
      m_postDeskewAngle(el.attribute("postDeskewAngle").toDouble()) {}

QDomElement DewarpingOptions::toXml(QDomDocument& doc, const QString& name) const {
  QDomElement el(doc.createElement(name));
  el.setAttribute("mode", formatDewarpingMode(m_mode));
  el.setAttribute("postDeskew", m_needPostDeskew ? "1" : "0");
  el.setAttribute("postDeskewAngle", Utils::doubleToString(m_postDeskewAngle));
  return el;
}

bool DewarpingOptions::operator==(const DewarpingOptions& other) const {
  return (m_mode == other.m_mode) && (m_needPostDeskew == other.m_needPostDeskew);
}

bool DewarpingOptions::operator!=(const DewarpingOptions& other) const {
  return !(*this == other);
}

DewarpingMode DewarpingOptions::parseDewarpingMode(const QString& str) {
  if (str == "auto") {
    return AUTO;
  } else if (str == "manual") {
    return MANUAL;
  } else if (str == "marginal") {
    return MARGINAL;
  } else {
    return OFF;
  }
}

QString DewarpingOptions::formatDewarpingMode(DewarpingMode mode) {
  switch (mode) {
    case OFF:
      return "off";
    case AUTO:
      return "auto";
    case MANUAL:
      return "manual";
    case MARGINAL:
      return "marginal";
  }

  assert(!"Unreachable");
  return QString();
}
}  // namespace output