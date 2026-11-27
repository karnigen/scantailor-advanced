// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <memory>

class QDomDocument;
class QDomElement;
class QString;

class Property {
 public:
  virtual ~Property() = default;

  virtual std::shared_ptr<Property> clone() const = 0;

  virtual QDomElement toXml(QDomDocument& doc, const QString& name) const = 0;
};
