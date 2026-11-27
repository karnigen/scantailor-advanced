// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#ifndef SCANTAILOR_OUTPUT_THUMBNAIL_H_
#define SCANTAILOR_OUTPUT_THUMBNAIL_H_

#include <memory>

#include "ThumbnailBase.h"

class ThumbnailPixmapCache;
class ImageTransformation;
class ImageId;
class QSizeF;

namespace output {
class Thumbnail : public ThumbnailBase {
 public:
  Thumbnail(std::shared_ptr<ThumbnailPixmapCache> thumbnailCache,
            const QSizeF& maxSize,
            const ImageId& imageId,
            const ImageTransformation& xform);
};
}  // namespace output
#endif
