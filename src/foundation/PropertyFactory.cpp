// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#include "PropertyFactory.h"

#include <QDomElement>

void PropertyFactory::registerProperty(const QString& property, PropertyConstructor constructor) {
  m_registry[property] = constructor;
}

std::shared_ptr<Property> PropertyFactory::construct(const QDomElement& el) const {
  auto it(m_registry.find(el.attribute("type")));
  if (it != m_registry.end()) {
    return (*it->second)(el);
  } else {
    return nullptr;
  }
}
