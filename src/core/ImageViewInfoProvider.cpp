// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#include "ImageViewInfoProvider.h"

#include <QtCore/QMutexLocker>
#include <QtCore/QRectF>

#include "Units.h"

ImageViewInfoProvider::ImageViewInfoProvider(const Dpi& dpi) : m_dpi(dpi) {}

ImageViewInfoProvider::~ImageViewInfoProvider() {
  notifyProviderStopped();
}

void ImageViewInfoProvider::addListener(ImageViewInfoListener* listener) {
  listener->onDpiChanged(m_dpi);
  listener->onPhysSizeChanged(m_physSize);
  listener->onMousePosChanged(m_mousePos);

  m_listeners.push_back(listener);
}

void ImageViewInfoProvider::removeAllListeners() {
  notifyProviderStopped();
  m_listeners.clear();
}

void ImageViewInfoProvider::setPhysSize(const QSizeF& physSize) {
  ImageViewInfoProvider::m_physSize = physSize;
  physSizeChanged(physSize);
}

void ImageViewInfoProvider::setMousePos(const QPointF& mousePos) {
  ImageViewInfoProvider::m_mousePos = mousePos;
  mousePosChanged(mousePos);
}

void ImageViewInfoProvider::physSizeChanged(const QSizeF& physSize) const {
  for (ImageViewInfoListener* listener : m_listeners) {
    listener->onPhysSizeChanged(physSize);
  }
}

void ImageViewInfoProvider::mousePosChanged(const QPointF& mousePos) const {
  for (ImageViewInfoListener* listener : m_listeners) {
    listener->onMousePosChanged(mousePos);
  }
}

const Dpi& ImageViewInfoProvider::getDpi() const {
  return m_dpi;
}

const QPointF& ImageViewInfoProvider::getMousePos() const {
  return m_mousePos;
}

const QSizeF& ImageViewInfoProvider::getPhysSize() const {
  return m_physSize;
}

void ImageViewInfoProvider::notifyProviderStopped() const {
  for (ImageViewInfoListener* listener : m_listeners) {
    listener->onImageViewInfoProviderStopped();
  }
}
