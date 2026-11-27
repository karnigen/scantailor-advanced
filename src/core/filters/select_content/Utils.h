// SPDX-FileCopyrightText: 2020 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2020 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#ifndef SCANTAILOR_SELECT_CONTENT_UTILS_H_
#define SCANTAILOR_SELECT_CONTENT_UTILS_H_

class Dpi;

namespace select_content {
class Params;

class Utils {
 public:
  Utils() = delete;

  static Params buildDefaultParams(const Dpi& dpi);
};
}  // namespace select_content

#endif  // SCANTAILOR_SELECT_CONTENT_UTILS_H_
