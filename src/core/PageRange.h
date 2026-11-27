// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#ifndef SCANTAILOR_CORE_PAGERANGE_H_
#define SCANTAILOR_CORE_PAGERANGE_H_

#include <set>
#include <vector>

#include "PageId.h"

class PageRange {
 public:
  /**
   * \brief Ordered list of consecutive pages.
   */
  std::vector<PageId> pages;

  std::set<PageId> selectEveryOther(const PageId& base) const;
};


#endif
