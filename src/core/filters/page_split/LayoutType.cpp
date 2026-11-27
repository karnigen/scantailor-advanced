// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#include "LayoutType.h"

#include <cassert>

namespace page_split {
QString layoutTypeToString(const LayoutType layoutType) {
  switch (layoutType) {
    case AUTO_LAYOUT_TYPE:
      return "auto-detect";
    case SINGLE_PAGE_UNCUT:
      return "single-uncut";
    case PAGE_PLUS_OFFCUT:
      return "single-cut";
    case TWO_PAGES:
      return "two-pages";
  }
  assert(!"unreachable");
  return QString();
}

LayoutType layoutTypeFromString(const QString& layoutType) {
  if (layoutType == "single-uncut") {
    return SINGLE_PAGE_UNCUT;
  } else if (layoutType == "single-cut") {
    return PAGE_PLUS_OFFCUT;
  } else if (layoutType == "two-pages") {
    return TWO_PAGES;
  } else {
    return AUTO_LAYOUT_TYPE;
  }
}
}  // namespace page_split