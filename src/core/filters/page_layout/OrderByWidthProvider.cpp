// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#include "OrderByWidthProvider.h"

#include <PageSizeOrderSnapshot.h>

#include <utility>

namespace page_layout {
OrderByWidthProvider::OrderByWidthProvider(std::shared_ptr<Settings> settings) : m_settings(std::move(settings)) {}

bool OrderByWidthProvider::precedes(const PageId& lhsPage,
                                    const bool lhsIncomplete,
                                    const PageId& rhsPage,
                                    const bool rhsIncomplete) const {
  return PageSizeOrderSnapshot::comparesBefore(m_settings->getOrderSizeMM(lhsPage), lhsIncomplete,
                                               m_settings->getOrderSizeMM(rhsPage), rhsIncomplete,
                                               PageSizeOrderSnapshot::Axis::WIDTH);
}

std::shared_ptr<const PageOrderProvider> OrderByWidthProvider::snapshot() const {
  return std::make_shared<PageSizeOrderSnapshot>(m_settings->getOrderSizesSnapshot(),
                                                 PageSizeOrderSnapshot::Axis::WIDTH);
}
}  // namespace page_layout
