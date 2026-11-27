// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#ifndef SCANTAILOR_CORE_FONTICONPACK_H_
#define SCANTAILOR_CORE_FONTICONPACK_H_

#include <foundation/Hashes.h>

#include <QString>
#include <unordered_map>

#include "AbstractIconPack.h"

class QDomElement;

class FontIconPack : public AbstractIconPack {
 public:
  static std::unique_ptr<FontIconPack> createDefault();

  explicit FontIconPack(const QString& iconSheetPath);

  QIcon getIcon(const QString& iconKey) const override;

 private:
  void addIconFromElement(const QDomElement& iconElement);

  std::unordered_map<QString, QIcon, hashes::hash<QString>> m_iconMap;
};


#endif  // SCANTAILOR_CORE_FONTICONPACK_H_
