// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#ifndef SCANTAILOR_CORE_PNGMETADATALOADER_H_
#define SCANTAILOR_CORE_PNGMETADATALOADER_H_

#include <vector>

#include "ImageMetadataLoader.h"
#include "VirtualFunction.h"

class QIODevice;
class ImageMetadata;

class PngMetadataLoader : public ImageMetadataLoader {
 protected:
  Status loadMetadata(QIODevice& ioDevice, const VirtualFunction<void, const ImageMetadata&>& out) override;
};


#endif
