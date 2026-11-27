// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#pragma once


#include <foundation/NonCopyable.h>

#include <QtGui/QIcon>
#include <memory>

class IconPack;

class IconProvider {
  DECLARE_NON_COPYABLE(IconProvider)
 private:
  IconProvider();

  ~IconProvider();

 public:
  static IconProvider& getInstance();

  void setIconPack(std::unique_ptr<IconPack> pack);

  void addIconPack(std::unique_ptr<IconPack> pack);

  QIcon getIcon(const QString& iconKey) const;

 private:
  std::unique_ptr<IconPack> m_iconPack;
};
