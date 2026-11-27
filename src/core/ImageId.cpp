// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#include "ImageId.h"

ImageId::ImageId(std::filesystem::path filePath, const int page) : m_filePath(std::move(filePath)), m_page(page) {}

bool operator==(const ImageId& lhs, const ImageId& rhs) {
  return ((lhs.page() == rhs.page()) && (lhs.filePath() == rhs.filePath()));
}

bool operator!=(const ImageId& lhs, const ImageId& rhs) {
  return !(lhs == rhs);
}

bool operator<(const ImageId& lhs, const ImageId& rhs) {
  const int comp = lhs.filePath().compare(rhs.filePath());
  if (comp < 0) {
    return true;
  } else if (comp > 0) {
    return false;
  }
  return lhs.page() < rhs.page();
}
