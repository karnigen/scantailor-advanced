// SPDX-FileCopyrightText: 2020 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2020 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

class Dpi;

namespace select_content {
class Params;

class Utils {
 public:
  Utils() = delete;

  static Params buildDefaultParams(const Dpi& dpi);
};
}  // namespace select_content
