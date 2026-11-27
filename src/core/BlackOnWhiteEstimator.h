// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <imageproc/BinaryImage.h>

#include <QtGui/QPolygonF>

#include "ImageTransformation.h"

class TaskStatus;
class DebugImages;

namespace imageproc {
class GrayImage;
class BinaryImage;
}  // namespace imageproc

class BlackOnWhiteEstimator {
 public:
  static bool isBlackOnWhite(const imageproc::GrayImage& grayImage,
                             const ImageTransformation& xform,
                             const TaskStatus& status,
                             DebugImages* dbg = nullptr);

  static bool isBlackOnWhiteRefining(const imageproc::GrayImage& grayImage,
                                     const ImageTransformation& xform,
                                     const TaskStatus& status,
                                     DebugImages* dbg = nullptr);

  static bool isBlackOnWhite(const imageproc::GrayImage& img, const imageproc::BinaryImage& mask);

  static bool isBlackOnWhite(const imageproc::GrayImage& img, const QPolygonF& cropArea);
};
