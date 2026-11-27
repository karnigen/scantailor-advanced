// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include "OutputImage.h"

namespace output {
class OutputImageWithForeground : public virtual OutputImage {
 public:
  virtual QImage getForegroundImage() const = 0;

  virtual QImage getBackgroundImage() const = 0;
};
}  // namespace output
