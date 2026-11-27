// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#include "Params.h"

#include <QDomDocument>

namespace page_split {
Params::Params(const PageLayout& layout, const Dependencies& deps, const AutoManualMode splitLineMode)
    : m_layout(layout), m_deps(deps), m_splitLineMode(splitLineMode) {}

Params::Params(const QDomElement& el)
    : m_layout(el.namedItem("pages").toElement()),
      m_deps(el.namedItem("dependencies").toElement()),
      m_splitLineMode(el.attribute("mode") == "manual" ? MODE_MANUAL : MODE_AUTO) {}

Params::~Params() = default;

QDomElement Params::toXml(QDomDocument& doc, const QString& name) const {
  QDomElement el(doc.createElement(name));
  el.setAttribute("mode", m_splitLineMode == MODE_AUTO ? "auto" : "manual");
  el.appendChild(m_layout.toXml(doc, "pages"));
  el.appendChild(m_deps.toXml(doc, "dependencies"));
  return el;
}
}  // namespace page_split