// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#include "TiffMetadataLoader.h"

#include "TiffReader.h"

ImageMetadataLoader::Status TiffMetadataLoader::loadMetadata(QIODevice& ioDevice,
                                                             const VirtualFunction<void, const ImageMetadata&>& out) {
  return TiffReader::readMetadata(ioDevice, out);
}
