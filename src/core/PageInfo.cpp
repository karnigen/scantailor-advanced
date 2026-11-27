// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#include "PageInfo.h"

PageInfo::PageInfo() : m_imageSubPages(0), m_leftHalfRemoved(false), m_rightHalfRemoved(false) {}

PageInfo::PageInfo(const PageId& pageId,
                   const ImageMetadata& metadata,
                   int imageSubPages,
                   bool leftHalfRemoved,
                   bool rightHalfRemoved)
    : m_pageId(pageId),
      m_metadata(metadata),
      m_imageSubPages(imageSubPages),
      m_leftHalfRemoved(leftHalfRemoved),
      m_rightHalfRemoved(rightHalfRemoved) {}
