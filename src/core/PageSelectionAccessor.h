// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#ifndef SCANTAILOR_CORE_PAGESELECTIONACCESSOR_H_
#define SCANTAILOR_CORE_PAGESELECTIONACCESSOR_H_

#include <memory>
#include <set>
#include <vector>

#include "PageId.h"
#include "PageRange.h"
#include "PageSelectionProvider.h"

class PageSequence;

class PageSelectionAccessor {
  // Member-wise copying is OK.
 public:
  explicit PageSelectionAccessor(std::shared_ptr<const PageSelectionProvider> provider);

  PageSequence allPages() const;

  std::set<PageId> selectedPages() const;

  std::vector<PageRange> selectedRanges() const;

 private:
  std::shared_ptr<const PageSelectionProvider> m_provider;
};


#endif
