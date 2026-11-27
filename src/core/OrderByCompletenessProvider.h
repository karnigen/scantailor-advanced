// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#ifndef SCANTAILOR_CORE_ORDERBYCOMPLETENESSPROVIDER_H_
#define SCANTAILOR_CORE_ORDERBYCOMPLETENESSPROVIDER_H_

#include "PageOrderProvider.h"

class OrderByCompletenessProvider : public PageOrderProvider {
 public:
  OrderByCompletenessProvider() = default;

  bool precedes(const PageId&, bool lhsIncomplete, const PageId&, bool rhsIncomplete) const override;

  // Ordering depends only on the comparison arguments, so there is nothing to freeze.
  std::shared_ptr<const PageOrderProvider> snapshot() const override { return shared_from_this(); }
};


#endif  // SCANTAILOR_CORE_ORDERBYCOMPLETENESSPROVIDER_H_
