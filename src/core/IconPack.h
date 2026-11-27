// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#ifndef SCANTAILOR_CORE_ICONPACK_H_
#define SCANTAILOR_CORE_ICONPACK_H_

#include <QIcon>
#include <memory>

class QString;

class IconPack {
 public:
  virtual ~IconPack() = default;

  virtual QIcon getIcon(const QString& iconKey) const = 0;

  virtual void mergeWith(std::unique_ptr<IconPack> pack) = 0;
};

#endif  // SCANTAILOR_CORE_ICONPACK_H_
