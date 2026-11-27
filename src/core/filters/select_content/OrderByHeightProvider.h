// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <memory>

#include "PageOrderProvider.h"
#include "Settings.h"

namespace select_content {
class OrderByHeightProvider : public PageOrderProvider {
 public:
  explicit OrderByHeightProvider(std::shared_ptr<Settings> settings);

  bool precedes(const PageId& lhsPage, bool lhsIncomplete, const PageId& rhsPage, bool rhsIncomplete) const override;

  std::shared_ptr<const PageOrderProvider> snapshot() const override;

 private:
  std::shared_ptr<Settings> m_settings;
};
}  // namespace select_content
