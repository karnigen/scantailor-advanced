// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <PropertySet.h>

#include "SerializableSpline.h"

class PropertyFactory;
class QDomDocument;
class QDomElement;
class QString;

class Zone {
  // Member-wise copying is OK, but that will produce a partly shallow copy.
 public:
  explicit Zone(const SerializableSpline& spline, const PropertySet& props = PropertySet());

  Zone(const QDomElement& el, const PropertyFactory& propFactory);

  QDomElement toXml(QDomDocument& doc, const QString& name) const;

  const SerializableSpline& spline() const;

  PropertySet& properties();

  const PropertySet& properties() const;

  bool isValid() const;

 private:
  SerializableSpline m_spline;
  PropertySet m_props;
};


inline const SerializableSpline& Zone::spline() const {
  return m_spline;
}

inline PropertySet& Zone::properties() {
  return m_props;
}

inline const PropertySet& Zone::properties() const {
  return m_props;
}
