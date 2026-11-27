// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#ifndef SCANTAILOR_IMAGEPROC_SCALE_H_
#define SCANTAILOR_IMAGEPROC_SCALE_H_

class QSize;

namespace imageproc {
class GrayImage;

/**
 * \brief Converts an image to grayscale and scales it to dstSize.
 *
 * \param src The source image.
 * \param dstSize The size to scale the image to.
 * \return The scaled image.
 *
 * This function is a faster replacement for QImage::scaled(), when
 * dealing with grayscale images. Downscaling uses area-weighted averaging
 * with fixed-point coordinates, so results may differ from
 * QImage::scaled()'s smooth transformation.
 */
GrayImage scaleToGray(const GrayImage& src, const QSize& dstSize);
}  // namespace imageproc
#endif
