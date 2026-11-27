// SPDX-FileCopyrightText: 2020 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2020 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#ifndef SCANTAILOR_PAGE_SPLIT_UTILS_H_
#define SCANTAILOR_PAGE_SPLIT_UTILS_H_

#include "Settings.h"

namespace page_split {
class Utils {
 public:
  Utils() = delete;

  static Settings::UpdateAction buildDefaultUpdateAction();
};
}  // namespace page_split

#endif  // SCANTAILOR_PAGE_SPLIT_UTILS_H_
