// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <QString>

namespace page_split {
enum LayoutType { AUTO_LAYOUT_TYPE, SINGLE_PAGE_UNCUT, PAGE_PLUS_OFFCUT, TWO_PAGES };

QString layoutTypeToString(LayoutType type);

LayoutType layoutTypeFromString(const QString& layoutType);
}  // namespace page_split
