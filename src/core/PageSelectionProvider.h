// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#ifndef SCANTAILOR_CORE_PAGESELECTIONPROVIDER_H_
#define SCANTAILOR_CORE_PAGESELECTIONPROVIDER_H_

#include <set>
#include <vector>


class PageSequence;
class PageId;
class PageRange;

class PageSelectionProvider {
 public:
  virtual ~PageSelectionProvider() = default;

  virtual PageSequence allPages() const = 0;

  virtual std::set<PageId> selectedPages() const = 0;

  virtual std::vector<PageRange> selectedRanges() const = 0;
};


#endif
