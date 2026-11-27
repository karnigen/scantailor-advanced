// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <memory>

#include "OutputImageWithForegroundMask.h"
#include "OutputImageWithOriginalBackground.h"

namespace output {
class OutputImageWithOriginalBackgroundMask : public OutputImageWithForegroundMask,
                                              public virtual OutputImageWithOriginalBackground {
 public:
  static std::unique_ptr<OutputImageWithOriginalBackgroundMask> fromPlainData(const QImage& foregroundImage,
                                                                              const QImage& backgroundImage,
                                                                              const QImage& originalBackground);

  OutputImageWithOriginalBackgroundMask() = default;

  OutputImageWithOriginalBackgroundMask(const QImage& image,
                                        const imageproc::BinaryImage& foregroundMask,
                                        ForegroundType foregroundType,
                                        const imageproc::BinaryImage& backgroundMask);

  bool isNull() const override;

  QImage getBackgroundImage() const override;

  QImage getOriginalBackgroundImage() const override;

 private:
  imageproc::BinaryImage m_backgroundMask;
};
}  // namespace output
