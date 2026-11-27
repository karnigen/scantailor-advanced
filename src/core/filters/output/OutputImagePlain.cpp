// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#include "OutputImagePlain.h"

#include <imageproc/Dpm.h>

namespace output {
OutputImagePlain::OutputImagePlain(const QImage& image) : m_image(image) {}

QImage OutputImagePlain::toImage() const {
  return m_image;
}

bool OutputImagePlain::isNull() const {
  return m_image.isNull();
}

void OutputImagePlain::setDpm(const Dpm& dpm) {
  m_image.setDotsPerMeterX(dpm.horizontal());
  m_image.setDotsPerMeterY(dpm.vertical());
}
}  // namespace output