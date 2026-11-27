// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#ifndef SCANTAILOR_IMAGEPROC_BWCOLOR_H_
#define SCANTAILOR_IMAGEPROC_BWCOLOR_H_

namespace imageproc {
enum BWColor { WHITE = 0, BLACK = 1 };

inline BWColor operator!(BWColor c) {
  return static_cast<BWColor>(~c & 1);
}
}  // namespace imageproc
#endif
