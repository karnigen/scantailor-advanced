// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#include "OrderByWidthProvider.h"

#include <PageSizeOrderSnapshot.h>

#include <utility>

namespace select_content {
OrderByWidthProvider::OrderByWidthProvider(std::shared_ptr<Settings> settings) : m_settings(std::move(settings)) {}

bool OrderByWidthProvider::precedes(const PageId& lhsPage,
                                    const bool lhsIncomplete,
                                    const PageId& rhsPage,
                                    const bool rhsIncomplete) const {
  return PageSizeOrderSnapshot::comparesBefore(m_settings->getContentSizeMM(lhsPage), lhsIncomplete,
                                               m_settings->getContentSizeMM(rhsPage), rhsIncomplete,
                                               PageSizeOrderSnapshot::Axis::WIDTH);
}

std::shared_ptr<const PageOrderProvider> OrderByWidthProvider::snapshot() const {
  return std::make_shared<PageSizeOrderSnapshot>(m_settings->getContentSizesSnapshot(),
                                                 PageSizeOrderSnapshot::Axis::WIDTH);
}
}  // namespace select_content
