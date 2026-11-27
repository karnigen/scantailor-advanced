// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#ifndef SCANTAILOR_IMAGEPROC_IMAGECOMBINATION_H_
#define SCANTAILOR_IMAGEPROC_IMAGECOMBINATION_H_

#include "BWColor.h"

class QImage;

namespace imageproc {
class BinaryImage;

void combineImages(QImage& mixedImage, const BinaryImage& foreground);

void combineImages(QImage& mixedImage, const BinaryImage& foreground, const BinaryImage& mask);

void combineImages(QImage& mixedImage, const QImage& foreground);

void combineImages(QImage& mixedImage, const QImage& foreground, const BinaryImage& mask);

void applyMask(QImage& image, const BinaryImage& bwMask, BWColor fillingColor = WHITE);
}  // namespace imageproc


#endif  // SCANTAILOR_IMAGEPROC_IMAGECOMBINATION_H_
