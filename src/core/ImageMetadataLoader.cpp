// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#include "ImageMetadataLoader.h"

#include <QFile>
#include <QIODevice>

#include "ImageMetadata.h"
#include "JpegMetadataLoader.h"
#include "PathUtils.h"
#include "PngMetadataLoader.h"
#include "TiffMetadataLoader.h"

ImageMetadataLoader::LoaderList ImageMetadataLoader::m_sLoaders;

void ImageMetadataLoader::registerLoader(std::shared_ptr<ImageMetadataLoader> loader) {
  m_sLoaders.push_back(std::move(loader));
}

ImageMetadataLoader::StaticInit::StaticInit() {
  registerLoader(std::make_shared<JpegMetadataLoader>());
  registerLoader(std::make_shared<PngMetadataLoader>());
  registerLoader(std::make_shared<TiffMetadataLoader>());
}

ImageMetadataLoader::StaticInit ImageMetadataLoader::m_staticInit;

ImageMetadataLoader::Status ImageMetadataLoader::loadImpl(QIODevice& ioDevice,
                                                          const VirtualFunction<void, const ImageMetadata&>& out) {
  auto it(m_sLoaders.begin());
  const auto end(m_sLoaders.end());
  for (; it != end; ++it) {
    const Status status = (*it)->loadMetadata(ioDevice, out);
    if (status != FORMAT_NOT_RECOGNIZED) {
      return status;
    }
  }
  return FORMAT_NOT_RECOGNIZED;
}

ImageMetadataLoader::Status ImageMetadataLoader::loadImpl(const std::filesystem::path& filePath,
                                                          const VirtualFunction<void, const ImageMetadata&>& out) {
  QFile file(core::toQString(filePath));
  if (!file.open(QIODevice::ReadOnly)) {
    return GENERIC_ERROR;
  }
  return loadImpl(file, out);
}