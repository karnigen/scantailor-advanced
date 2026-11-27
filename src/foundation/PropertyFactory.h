// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#ifndef SCANTAILOR_FOUNDATION_PROPERTYFACTORY_H_
#define SCANTAILOR_FOUNDATION_PROPERTYFACTORY_H_

#include <QString>
#include <memory>
#include <unordered_map>

#include "Hashes.h"
#include "Property.h"

class QDomElement;

class PropertyFactory {
  // Member-wise copying is OK.
 public:
  virtual ~PropertyFactory() = default;

  using PropertyConstructor = std::shared_ptr<Property> (*)(const QDomElement& el);

  void registerProperty(const QString& property, PropertyConstructor constructor);

  std::shared_ptr<Property> construct(const QDomElement& el) const;

 private:
  using Registry = std::unordered_map<QString, PropertyConstructor, hashes::hash<QString>>;
  Registry m_registry;
};


#endif
