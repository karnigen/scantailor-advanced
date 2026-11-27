// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#include "PageRange.h"

std::set<PageId> PageRange::selectEveryOther(const PageId& base) const {
  std::set<PageId> selection;

  auto it(pages.begin());
  const auto end(pages.end());
  for (; it != end && *it != base; ++it) {
    // Continue until we have a match.
  }
  if (it == end) {
    return selection;
  }

  const int baseIdx = static_cast<int>(it - pages.begin());
  int idx = 0;
  for (const PageId& pageId : pages) {
    if (((idx - baseIdx) & 1) == 0) {
      selection.insert(pageId);
    }
    ++idx;
  }
  return selection;
}
