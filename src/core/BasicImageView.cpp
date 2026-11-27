// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#include "BasicImageView.h"

#include "Dpm.h"
#include "ImagePresentation.h"
#include "ImageTransformation.h"

BasicImageView::BasicImageView(const QImage& image, const ImagePixmapUnion& downscaledImage, const Margins& margins)
    : ImageViewBase(image, downscaledImage, ImagePresentation(QTransform(), QRectF(image.rect())), margins),
      m_dragHandler(*this),
      m_zoomHandler(*this) {
  rootInteractionHandler().makeLastFollower(m_dragHandler);
  rootInteractionHandler().makeLastFollower(m_zoomHandler);
}

BasicImageView::~BasicImageView() = default;
