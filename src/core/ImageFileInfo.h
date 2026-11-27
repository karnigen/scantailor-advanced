// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_CORE_IMAGEFILEINFO_H_
#define SCANTAILOR_CORE_IMAGEFILEINFO_H_

#include <QFileInfo>
#include <cassert>
#include <utility>
#include <vector>

#include "ImageMetadata.h"

class ImageFileInfo {
  // Member-wise copying is OK.
 public:
  ImageFileInfo(const QFileInfo& fileInfo,
                const std::vector<ImageMetadata>& imageInfo,
                std::vector<int> imageIndexes = {})
      : m_fileInfo(fileInfo), m_imageInfo(imageInfo), m_imageIndexes(std::move(imageIndexes)) {
    if (m_imageIndexes.empty()) {
      for (std::size_t index = 0; index < m_imageInfo.size(); ++index) {
        m_imageIndexes.push_back(static_cast<int>(index));
      }
    } else {
      assert(m_imageIndexes.size() == m_imageInfo.size());
    }
  }

  const QFileInfo& fileInfo() const { return m_fileInfo; }

  std::vector<ImageMetadata>& imageInfo() { return m_imageInfo; }

  const std::vector<ImageMetadata>& imageInfo() const { return m_imageInfo; }

  const std::vector<int>& imageIndexes() const { return m_imageIndexes; }

  bool isDpiOK() const;

 private:
  QFileInfo m_fileInfo;
  std::vector<ImageMetadata> m_imageInfo;
  std::vector<int> m_imageIndexes;
};


#endif  // ifndef SCANTAILOR_CORE_IMAGEFILEINFO_H_
