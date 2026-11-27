// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#include "OutputProcessingParams.h"

#include <QDomDocument>

#include "BlackWhiteOptions.h"

namespace output {

OutputProcessingParams::OutputProcessingParams() : m_autoZonesFound(false), m_blackOnWhiteSetManually(false) {}

OutputProcessingParams::OutputProcessingParams(const QDomElement& el)
    : m_autoZonesFound(el.attribute("autoZonesFound") == "1"),
      m_blackOnWhiteSetManually(el.attribute("blackOnWhiteSetManually") == "1") {}

QDomElement OutputProcessingParams::toXml(QDomDocument& doc, const QString& name) const {
  QDomElement el(doc.createElement(name));
  el.setAttribute("autoZonesFound", m_autoZonesFound ? "1" : "0");
  el.setAttribute("blackOnWhiteSetManually", m_blackOnWhiteSetManually ? "1" : "0");
  return el;
}

bool OutputProcessingParams::operator==(const OutputProcessingParams& other) const {
  return (m_autoZonesFound == other.m_autoZonesFound) && (m_blackOnWhiteSetManually == other.m_blackOnWhiteSetManually);
}

bool OutputProcessingParams::operator!=(const OutputProcessingParams& other) const {
  return !(*this == other);
}
}  // namespace output