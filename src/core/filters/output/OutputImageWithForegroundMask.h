// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <imageproc/BinaryImage.h>

#include <memory>

#include "ForegroundType.h"
#include "OutputImagePlain.h"
#include "OutputImageWithForeground.h"

namespace output {
class OutputImageWithForegroundMask : public OutputImagePlain, public virtual OutputImageWithForeground {
 public:
  static std::unique_ptr<OutputImageWithForegroundMask> fromPlainData(const QImage& foregroundImage,
                                                                      const QImage& backgroundImage);

  OutputImageWithForegroundMask() = default;

  OutputImageWithForegroundMask(const QImage& image, const imageproc::BinaryImage& mask, ForegroundType type);

  bool isNull() const override;

  QImage getForegroundImage() const override;

  QImage getBackgroundImage() const override;

 protected:
  static ForegroundType getForegroundType(const QImage& foregroundImage);

 private:
  imageproc::BinaryImage m_foregroundMask;
  ForegroundType m_foregroundType = ForegroundType::COLOR;
};
}  // namespace output
