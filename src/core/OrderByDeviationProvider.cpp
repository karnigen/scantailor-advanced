// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#include "OrderByDeviationProvider.h"

#include <unordered_map>

namespace {
class DeviationOrderSnapshot : public PageOrderProvider {
 public:
  explicit DeviationOrderSnapshot(std::unordered_map<PageId, double> deviations)
      : m_deviations(std::move(deviations)) {}

  bool precedes(const PageId& lhsPage, bool lhsIncomplete, const PageId& rhsPage, bool rhsIncomplete) const override {
    if (lhsIncomplete != rhsIncomplete) {
      return lhsIncomplete;
    }
    return deviationValue(lhsPage) > deviationValue(rhsPage);
  }

  std::shared_ptr<const PageOrderProvider> snapshot() const override { return shared_from_this(); }

 private:
  double deviationValue(const PageId& page) const {
    const auto it = m_deviations.find(page);
    return it == m_deviations.end() ? -1.0 : it->second;
  }

  const std::unordered_map<PageId, double> m_deviations;
};
}  // namespace

OrderByDeviationProvider::OrderByDeviationProvider(const DeviationProvider<PageId>& deviationProvider)
    : m_deviationProvider(&deviationProvider) {}

bool OrderByDeviationProvider::precedes(const PageId& lhsPage,
                                        bool lhsIncomplete,
                                        const PageId& rhsPage,
                                        bool rhsIncomplete) const {
  if (lhsIncomplete != rhsIncomplete) {
    // Invalid (unknown) sizes go to the back.
    return lhsIncomplete;
  }
  return (m_deviationProvider->getDeviationValue(lhsPage) > m_deviationProvider->getDeviationValue(rhsPage));
}

std::shared_ptr<const PageOrderProvider> OrderByDeviationProvider::snapshot() const {
  return std::make_shared<DeviationOrderSnapshot>(m_deviationProvider->getDeviationValuesSnapshot());
}
