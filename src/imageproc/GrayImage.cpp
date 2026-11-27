// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#include "GrayImage.h"

#include "Grayscale.h"

namespace imageproc {
GrayImage::GrayImage(QSize size) {
  if (size.isEmpty()) {
    return;
  }

  m_image = QImage(size, QImage::Format_Indexed8);
  m_image.setColorTable(createGrayscalePalette());
  if (m_image.isNull()) {
    throw std::bad_alloc();
  }
}

GrayImage::GrayImage(const QImage& image) : m_image(toGrayscale(image)) {}

GrayImage GrayImage::inverted() const {
  GrayImage inverted(*this);
  inverted.invert();
  return inverted;
}

void GrayImage::invert() {
  m_image.invertPixels(QImage::InvertRgb);
}

int GrayImage::dotsPerMeterX() const {
  return m_image.dotsPerMeterX();
}

int GrayImage::dotsPerMeterY() const {
  return m_image.dotsPerMeterY();
}

void GrayImage::setDotsPerMeterX(int value) {
  m_image.setDotsPerMeterX(value);
}

void GrayImage::setDotsPerMeterY(int value) {
  m_image.setDotsPerMeterY(value);
}
}  // namespace imageproc
