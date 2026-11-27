// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include "ImageId.h"
#include "ImageMetadata.h"

/**
 * This class stores the same information about an image as ProjectPages does,
 * and is used for adding images to ProjectPages objects.  Beyond that,
 * ProjectPages doesn't operate with ImageInfo objects, but with PageInfo ones.
 */
class ImageInfo {
  // Member-wise copying is OK.
 public:
  ImageInfo();

  ImageInfo(const ImageId& id,
            const ImageMetadata& metadata,
            int numSubPages,
            bool leftPageRemoved,
            bool rightPageRemoved);

  const ImageId& id() const { return m_id; }

  const ImageMetadata& metadata() const { return m_metadata; }

  int numSubPages() const { return m_numSubPages; }

  bool leftHalfRemoved() const { return m_leftHalfRemoved; }

  bool rightHalfRemoved() const { return m_rightHalfRemoved; }

 private:
  ImageId m_id;
  ImageMetadata m_metadata;
  int m_numSubPages;        // 1 or 2
  bool m_leftHalfRemoved;   // Both can't be true, and if one is true,
  bool m_rightHalfRemoved;  // then m_numSubPages is 1.
};
