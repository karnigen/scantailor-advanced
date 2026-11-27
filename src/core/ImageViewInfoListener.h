// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#ifndef SCANTAILOR_CORE_IMAGEVIEWINFOLISTENER_H_
#define SCANTAILOR_CORE_IMAGEVIEWINFOLISTENER_H_


#include <QtCore/QPointF>
#include <QtCore/QSizeF>

class Dpi;

class ImageViewInfoListener {
 public:
  virtual ~ImageViewInfoListener() = default;

  virtual void onMousePosChanged(const QPointF& mousePos) = 0;

  virtual void onPhysSizeChanged(const QSizeF& physSize) = 0;

  virtual void onDpiChanged(const Dpi& dpi) = 0;

  virtual void onImageViewInfoProviderStopped() = 0;
};


#endif  // SCANTAILOR_CORE_IMAGEVIEWINFOLISTENER_H_
