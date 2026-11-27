// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <memory>

#include "AbstractFilterDataCollector.h"

class ThumbnailPixmapCache;
class QGraphicsItem;
class QSizeF;

class ThumbnailCollector : public AbstractFilterDataCollector {
 public:
  virtual void processThumbnail(std::unique_ptr<QGraphicsItem>) = 0;

  virtual std::shared_ptr<ThumbnailPixmapCache> thumbnailCache() = 0;

  virtual QSizeF maxLogicalThumbSize() const = 0;
};
