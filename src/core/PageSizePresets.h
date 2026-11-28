// SPDX-FileCopyrightText: 2026 project contributors
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

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
