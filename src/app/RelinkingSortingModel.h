// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <QSortFilterProxyModel>

class RelinkingSortingModel : public QSortFilterProxyModel {
 public:
  explicit RelinkingSortingModel(QObject* parent = nullptr);

 protected:
  bool lessThan(const QModelIndex& left, const QModelIndex& right) const override;
};
