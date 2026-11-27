// Copyright (C) 2026  ScanTailor Advanced contributors
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_CORE_PAGESIZEPRESETS_H_
#define SCANTAILOR_CORE_PAGESIZEPRESETS_H_

#include <QSizeF>
#include <QString>

#include <vector>

namespace page_sizes {
struct Preset {
  QString name;
  QSizeF sizeMm;
};

std::vector<Preset> load();

bool sizesMatch(const QSizeF& lhs, const QSizeF& rhs);

QString matchingName(const QSizeF& sizeMm);
}  // namespace page_sizes

#endif  // SCANTAILOR_CORE_PAGESIZEPRESETS_H_
