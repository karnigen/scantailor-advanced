// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

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
