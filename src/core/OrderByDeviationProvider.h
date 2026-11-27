// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include "DeviationProvider.h"
#include "PageId.h"
#include "PageOrderProvider.h"

class OrderByDeviationProvider : public PageOrderProvider {
 public:
  explicit OrderByDeviationProvider(const DeviationProvider<PageId>& deviationProvider);

  bool precedes(const PageId& lhsPage, bool lhsIncomplete, const PageId& rhsPage, bool rhsIncomplete) const override;

  std::shared_ptr<const PageOrderProvider> snapshot() const override;

 private:
  const DeviationProvider<PageId>* m_deviationProvider;
};
