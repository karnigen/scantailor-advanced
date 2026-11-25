// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "OrderByHeightProvider.h"

#include <PageSizeOrderSnapshot.h>

#include <utility>

namespace page_layout {
OrderByHeightProvider::OrderByHeightProvider(std::shared_ptr<Settings> settings) : m_settings(std::move(settings)) {}

bool OrderByHeightProvider::precedes(const PageId& lhsPage,
                                     const bool lhsIncomplete,
                                     const PageId& rhsPage,
                                     const bool rhsIncomplete) const {
  return PageSizeOrderSnapshot::comparesBefore(m_settings->getOrderSizeMM(lhsPage), lhsIncomplete,
                                               m_settings->getOrderSizeMM(rhsPage), rhsIncomplete,
                                               PageSizeOrderSnapshot::Axis::HEIGHT);
}

std::shared_ptr<const PageOrderProvider> OrderByHeightProvider::snapshot() const {
  return std::make_shared<PageSizeOrderSnapshot>(m_settings->getOrderSizesSnapshot(),
                                                 PageSizeOrderSnapshot::Axis::HEIGHT);
}
}  // namespace page_layout
