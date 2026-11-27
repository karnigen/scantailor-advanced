// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <QImage>

class Dpm;

namespace output {
class OutputImage {
 public:
  virtual ~OutputImage() = default;

  virtual QImage toImage() const = 0;

  virtual bool isNull() const = 0;

  virtual void setDpm(const Dpm& dpm) = 0;

  virtual operator QImage() const { return toImage(); }
};
}  // namespace output
