// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#include "Zone.h"

#include <QDomDocument>

Zone::Zone(const SerializableSpline& spline, const PropertySet& props) : m_spline(spline), m_props(props) {}

Zone::Zone(const QDomElement& el, const PropertyFactory& propFactory)
    : m_spline(el.namedItem("spline").toElement()), m_props(el.namedItem("properties").toElement(), propFactory) {}

QDomElement Zone::toXml(QDomDocument& doc, const QString& name) const {
  QDomElement el(doc.createElement(name));
  el.appendChild(m_spline.toXml(doc, "spline"));
  el.appendChild(m_props.toXml(doc, "properties"));
  return el;
}

bool Zone::isValid() const {
  const QPolygonF& shape = m_spline.toPolygon();

  switch (shape.size()) {
    case 0:
    case 1:
    case 2:
      return false;
    case 3:
      if (shape.front() == shape.back()) {
        return false;
      }
      // fall through
    default:
      return true;
  }
}
