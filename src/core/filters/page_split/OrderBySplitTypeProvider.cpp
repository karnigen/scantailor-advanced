// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "OrderBySplitTypeProvider.h"

#include <cassert>
#include <utility>

namespace page_split {
namespace {
class SplitTypeOrderSnapshot : public PageOrderProvider {
 public:
  explicit SplitTypeOrderSnapshot(Settings::OrderLayoutTypesSnapshot snapshot) : m_snapshot(std::move(snapshot)) {}

  bool precedes(const PageId& lhsPage,
                const bool lhsIncomplete,
                const PageId& rhsPage,
                const bool rhsIncomplete) const override {
    if (lhsIncomplete != rhsIncomplete) {
      // Pages with question mark go to the bottom.
      return rhsIncomplete;
    }
    if (lhsIncomplete) {
      // Two pages with question marks are ordered naturally.
      return lhsPage < rhsPage;
    }

    const int lhsLayoutType = layoutTypeFor(lhsPage.imageId());
    const int rhsLayoutType = layoutTypeFor(rhsPage.imageId());
    return lhsLayoutType == rhsLayoutType ? (lhsPage < rhsPage) : (lhsLayoutType < rhsLayoutType);
  }

  std::shared_ptr<const PageOrderProvider> snapshot() const override { return shared_from_this(); }

 private:
  int layoutTypeFor(const ImageId& imageId) const {
    const auto it = m_snapshot.byImageId.find(imageId);
    return it == m_snapshot.byImageId.end() ? m_snapshot.defaultType : it->second;
  }

  const Settings::OrderLayoutTypesSnapshot m_snapshot;
};
}  // namespace

OrderBySplitTypeProvider::OrderBySplitTypeProvider(std::shared_ptr<Settings> settings)
    : m_settings(std::move(settings)) {}

bool OrderBySplitTypeProvider::precedes(const PageId& lhsPage,
                                        const bool lhsIncomplete,
                                        const PageId& rhsPage,
                                        const bool rhsIncomplete) const {
  if (lhsIncomplete != rhsIncomplete) {
    // Pages with question mark go to the bottom.
    return rhsIncomplete;
  } else if (lhsIncomplete) {
    assert(rhsIncomplete);
    // Two pages with question marks are ordered naturally.
    return lhsPage < rhsPage;
  }

  assert(!lhsIncomplete);
  assert(!rhsIncomplete);

  const int lhsLayoutType = m_settings->orderLayoutType(lhsPage.imageId());
  const int rhsLayoutType = m_settings->orderLayoutType(rhsPage.imageId());
  if (lhsLayoutType == rhsLayoutType) {
    return lhsPage < rhsPage;
  } else {
    return lhsLayoutType < rhsLayoutType;
  }
}

std::shared_ptr<const PageOrderProvider> OrderBySplitTypeProvider::snapshot() const {
  return std::make_shared<SplitTypeOrderSnapshot>(m_settings->getOrderLayoutTypesSnapshot());
}
}  // namespace page_split
