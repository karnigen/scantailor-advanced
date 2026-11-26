// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "RelinkingSortingModel.h"

#include "RelinkingModel.h"

RelinkingSortingModel::RelinkingSortingModel(QObject* parent) : QSortFilterProxyModel(parent) {
  setDynamicSortFilter(true);
  sort(0);
}

bool RelinkingSortingModel::lessThan(const QModelIndex& left, const QModelIndex& right) const {
  const int leftKind = left.data(RelinkingModel::EntryKindRole).toInt();
  const int rightKind = right.data(RelinkingModel::EntryKindRole).toInt();
  if (leftKind != rightKind) {
    return leftKind < rightKind;
  }

  const int leftStatus = left.data(RelinkingModel::UncommittedStatusRole).toInt();
  const int rightStatus = right.data(RelinkingModel::UncommittedStatusRole).toInt();
  if (leftStatus != rightStatus) {
    const auto statusPriority = [](const int status) {
      switch (status) {
        case RelinkingModel::Missing:
          return 0;
        case RelinkingModel::StatusUpdateFailed:
          return 1;
        case RelinkingModel::StatusUpdatePending:
          return 2;
        case RelinkingModel::Exists:
          return 3;
      }
      return 4;
    };
    return statusPriority(leftStatus) < statusPriority(rightStatus);
  }

  const int leftType = left.data(RelinkingModel::TypeRole).toInt();
  const int rightType = right.data(RelinkingModel::TypeRole).toInt();
  const QString leftPath(left.data(RelinkingModel::UncommittedPathRole).toString());
  const QString rightPath(right.data(RelinkingModel::UncommittedPathRole).toString());

  if (leftType != rightType) {
    // Directories go above their siblings.
    const QString leftDir(leftPath.left(leftPath.lastIndexOf(QChar('/'))));
    const QString rightDir(rightPath.left(rightPath.lastIndexOf(QChar('/'))));
    if (leftDir == rightDir) {
      return leftType == RelinkablePath::Dir;
    }
  }

  // The last resort is lexicographical ordering.
  return leftPath < rightPath;
}  // RelinkingSortingModel::lessThan
