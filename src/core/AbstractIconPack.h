// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#ifndef SCANTAILOR_CORE_ABSTRACTICONPACK_H_
#define SCANTAILOR_CORE_ABSTRACTICONPACK_H_

#include "IconPack.h"

class AbstractIconPack : public IconPack {
 public:
  QIcon getIcon(const QString& iconKey) const override;

  void mergeWith(std::unique_ptr<IconPack> pack) override;

 protected:
  static QIcon::Mode iconModeFromString(const QString& mode);

  static QIcon::State iconStateFromString(const QString& state);

 private:
  std::unique_ptr<IconPack> m_parentIconPack;
};


#endif  // SCANTAILOR_CORE_ABSTRACTICONPACK_H_
