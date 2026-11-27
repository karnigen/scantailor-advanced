// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#include "PageSelectionAccessor.h"

#include <utility>

#include "PageSequence.h"

PageSelectionAccessor::PageSelectionAccessor(std::shared_ptr<const PageSelectionProvider> provider)
    : m_provider(std::move(provider)) {}

PageSequence PageSelectionAccessor::allPages() const {
  return m_provider->allPages();
}

std::set<PageId> PageSelectionAccessor::selectedPages() const {
  return m_provider->selectedPages();
}

std::vector<PageRange> PageSelectionAccessor::selectedRanges() const {
  return m_provider->selectedRanges();
}
