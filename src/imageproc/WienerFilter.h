// SPDX-FileCopyrightText: 2015 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef SCANTAILOR_IMAGEPROC_WIENER_FILTER_H_
#define SCANTAILOR_IMAGEPROC_WIENER_FILTER_H_

#include "GrayImage.h"

class QImage;
class QSize;

namespace imageproc {

class GrayImage;

/**
 * @brief Applies the Wiener filter to a grayscale image.
 *
 * @param image The image to apply the filter to. A null image is allowed.
 * @param window_size The local neighbourhood around a pixel to use.
 * @param noise_sigma The standard deviation of noise in the image.
 * @return The filtered image.
 */
GrayImage wienerFilter(GrayImage const& image, QSize const& window_size, double noise_sigma);

/**
 * @brief An in-place version of wienerFilter().
 * @see wienerFilter()
 */
void wienerFilterInPlace(GrayImage& image, QSize const& window_size, double noise_sigma);


QImage wienerColorFilter(QImage const& image, QSize const& window_size, double coef);

void wienerColorFilterInPlace(QImage& image, QSize const& window_size, double coef);

}  // namespace imageproc

#endif
