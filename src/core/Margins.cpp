// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#include "Margins.h"

#include <foundation/Utils.h>

#include <QDomDocument>
#include <QDomElement>
#include <QString>

using namespace foundation;

Margins::Margins() : m_top(), m_bottom(), m_left(), m_right() {}

Margins::Margins(double left, double top, double right, double bottom)
    : m_top(top), m_bottom(bottom), m_left(left), m_right(right) {}

Margins::Margins(const QDomElement& el)
    : m_top(el.attribute("top").toDouble()),
      m_bottom(el.attribute("bottom").toDouble()),
      m_left(el.attribute("left").toDouble()),
      m_right(el.attribute("right").toDouble()) {}

QDomElement Margins::toXml(QDomDocument& doc, const QString& name) const {
  QDomElement el(doc.createElement(name));
  el.setAttribute("left", Utils::doubleToString(m_left));
  el.setAttribute("right", Utils::doubleToString(m_right));
  el.setAttribute("top", Utils::doubleToString(m_top));
  el.setAttribute("bottom", Utils::doubleToString(m_bottom));
  return el;
}