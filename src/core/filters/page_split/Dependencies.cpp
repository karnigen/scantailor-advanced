// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#include "Dependencies.h"

#include "Params.h"
#include "XmlMarshaller.h"
#include "XmlUnmarshaller.h"

namespace page_split {
Dependencies::Dependencies() : m_layoutType(AUTO_LAYOUT_TYPE) {}

Dependencies::Dependencies(const QDomElement& el)
    : m_imageSize(XmlUnmarshaller::size(el.namedItem("size").toElement())),
      m_rotation(el.namedItem("rotation").toElement()),
      m_layoutType(layoutTypeFromString(XmlUnmarshaller::string(el.namedItem("layoutType").toElement()))) {}

Dependencies::Dependencies(const QSize& imageSize, const OrthogonalRotation rotation, const LayoutType layoutType)
    : m_imageSize(imageSize), m_rotation(rotation), m_layoutType(layoutType) {}

bool Dependencies::compatibleWith(const Params& params) const {
  const Dependencies& deps = params.dependencies();

  if (m_imageSize != deps.m_imageSize) {
    return false;
  }
  if (m_rotation != deps.m_rotation) {
    return false;
  }
  if (m_layoutType == deps.m_layoutType) {
    return true;
  }
  if (m_layoutType == SINGLE_PAGE_UNCUT) {
    // The split line doesn't matter here.
    return true;
  }
  if ((m_layoutType == TWO_PAGES) && (params.splitLineMode() == MODE_MANUAL)) {
    // Two pages and a specified split line means we have all the data.
    // Note that if layout type was PAGE_PLUS_OFFCUT, we would
    // not know if that page is to the left or to the right of the
    // split line.
    return true;
  }
  return false;
}

QDomElement Dependencies::toXml(QDomDocument& doc, const QString& tagName) const {
  if (isNull()) {
    return QDomElement();
  }

  XmlMarshaller marshaller(doc);

  QDomElement el(doc.createElement(tagName));
  el.appendChild(m_rotation.toXml(doc, "rotation"));
  el.appendChild(marshaller.size(m_imageSize, "size"));
  el.appendChild(marshaller.string(layoutTypeToString(m_layoutType), "layoutType"));
  return el;
}

bool Dependencies::isNull() const {
  return m_imageSize.isNull();
}
}  // namespace page_split