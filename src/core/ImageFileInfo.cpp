// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#include "ImageFileInfo.h"

bool ImageFileInfo::isDpiOK() const {
  return std::find_if(m_imageInfo.begin(), m_imageInfo.end(),
                      [](const ImageMetadata& metadata) { return !metadata.isDpiOK(); })
         == m_imageInfo.end();
}
