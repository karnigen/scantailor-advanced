// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

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
