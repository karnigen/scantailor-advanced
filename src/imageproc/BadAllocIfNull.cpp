// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#include "BadAllocIfNull.h"

#include <QImage>
#include <new>

namespace imageproc {
const QImage& badAllocIfNull(const QImage& image) {
  if (image.isNull()) {
    throw std::bad_alloc();
  }
  return image;
}
}  // namespace imageproc
