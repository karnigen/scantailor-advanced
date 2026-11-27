// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#ifndef SCANTAILOR_IMAGEPROC_BADALLOCIFNULL_H_
#define SCANTAILOR_IMAGEPROC_BADALLOCIFNULL_H_

class QImage;

namespace imageproc {
/**
 * @brief Throw std::bad_alloc exception if the image is null.
 *
 * Qt has this annoying behaviour when an out-of-memory situation
 * in a method of QImage results in either a null image being returned
 * from that method or even "*this" image itself becoming null! Either
 * scenario leads to a crash with high probability. Instead, we prefer
 * to throw std::bad_alloc(), which will result in an "Out of Memory"
 * dialog and a graceful termination.
 *
 * @param image The image to test.
 * @return The image passed as the argument.
 */
const QImage& badAllocIfNull(const QImage& image);
}  // namespace imageproc

#endif
