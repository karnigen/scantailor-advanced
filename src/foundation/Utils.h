// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#ifndef SCANTAILOR_FOUNDATION_UTILS_H_
#define SCANTAILOR_FOUNDATION_UTILS_H_

#include <QString>

namespace foundation {
class Utils {
 public:
  /**
   * \brief A high precision, locale independent number to string conversion.
   *
   * This function is intended to be used instead of
   * QDomElement::setAttribute(double), which is locale dependent.
   */
  static QString doubleToString(double val);

  Utils() = delete;
};
}  // namespace foundation


#endif  // SCANTAILOR_FOUNDATION_UTILS_H_
