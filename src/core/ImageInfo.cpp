// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#include "ImageInfo.h"

ImageInfo::ImageInfo() : m_numSubPages(0), m_leftHalfRemoved(false), m_rightHalfRemoved(false) {}

ImageInfo::ImageInfo(const ImageId& id,
                     const ImageMetadata& metadata,
                     int numSubPages,
                     bool leftHalfRemoved,
                     bool rightHalfRemoved)
    : m_id(id),
      m_metadata(metadata),
      m_numSubPages(numSubPages),
      m_leftHalfRemoved(leftHalfRemoved),
      m_rightHalfRemoved(rightHalfRemoved) {}
