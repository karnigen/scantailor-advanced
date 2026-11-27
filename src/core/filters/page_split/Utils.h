// SPDX-FileCopyrightText: 2020 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2020 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include "Settings.h"

namespace page_split {
class Utils {
 public:
  Utils() = delete;

  static Settings::UpdateAction buildDefaultUpdateAction();
};
}  // namespace page_split
