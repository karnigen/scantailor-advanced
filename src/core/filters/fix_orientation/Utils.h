// SPDX-FileCopyrightText: 2020 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2020 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#ifndef SCANTAILOR_FIX_ORIENTATION_UTILS_H_
#define SCANTAILOR_FIX_ORIENTATION_UTILS_H_

class OrthogonalRotation;

namespace fix_orientation {
class Utils {
 public:
  Utils() = delete;

  static OrthogonalRotation getDefaultOrthogonalRotation();
};
}  // namespace fix_orientation

#endif  // SCANTAILOR_FIX_ORIENTATION_UTILS_H_
