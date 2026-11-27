// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#include "OrderByCompletenessProvider.h"

bool OrderByCompletenessProvider::precedes(const PageId&, bool lhsIncomplete, const PageId&, bool rhsIncomplete) const {
  if (lhsIncomplete != rhsIncomplete) {
    // Incomplete pages go to the back.
    return rhsIncomplete;
  }
  return true;
}
