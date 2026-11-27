// SPDX-FileCopyrightText: 2020 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2020 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#ifndef SCANTAILOR_DESKEW_UTILS_H_
#define SCANTAILOR_DESKEW_UTILS_H_

#include <core/DefaultParams.h>

namespace deskew {
class Params;

class Utils {
 public:
  Utils() = delete;

  static Params buildDefaultParams();

  static Params buildDefaultParams(const DefaultParams::DeskewParams& deskewParams);
};
}  // namespace deskew

#endif  // SCANTAILOR_DESKEW_UTILS_H_
