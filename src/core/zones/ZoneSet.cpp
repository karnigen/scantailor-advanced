// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#include "ZoneSet.h"

#include <QDomNode>

ZoneSet::ZoneSet(const QDomElement& el, const PropertyFactory& propFactory) {
  const QString zoneStr("zone");

  QDomNode node(el.firstChild());
  for (; !node.isNull(); node = node.nextSibling()) {
    if (!node.isElement()) {
      continue;
    }
    if (node.nodeName() != zoneStr) {
      continue;
    }

    const Zone zone(node.toElement(), propFactory);
    if (zone.isValid()) {
      m_zones.push_back(zone);
    }
  }
}

QDomElement ZoneSet::toXml(QDomDocument& doc, const QString& name) const {
  const QString zoneStr("zone");

  QDomElement el(doc.createElement(name));
  for (const Zone& zone : m_zones) {
    el.appendChild(zone.toXml(doc, zoneStr));
  }
  return el;
}
