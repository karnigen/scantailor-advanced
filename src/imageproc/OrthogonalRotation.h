// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only


#pragma once

class QRect;

namespace imageproc {
class BinaryImage;

/**
 * \brief Rotation by 0, 90, 180 or 270 degrees.
 *
 * \param src The source image.  May be null, in which case
 *        a null rotated image will be returned.
 * \param srcRect The area that is to be rotated.
 * \param degrees The rotation angle in degrees.  The angle
 *        must be a multiple of 90.  Positive values indicate
 *        clockwise rotation.
 * \return The rotated area of the source image.  The dimensions
 *         of the returned image will correspond to \p srcRect,
 *         possibly with width and height swapped.
 */
BinaryImage orthogonalRotation(const BinaryImage& src, const QRect& srcRect, int degrees);

/**
 * \brief Rotation by 90, 180 or 270 degrees.
 *
 * This is an overload provided for convenience.
 * It rotates the whole image, not a portion of it.
 */
BinaryImage orthogonalRotation(const BinaryImage& src, int degrees);
}  // namespace imageproc
