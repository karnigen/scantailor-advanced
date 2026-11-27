// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#ifndef SCANTAILOR_CORE_IMAGEPIXMAPUNION_H_
#define SCANTAILOR_CORE_IMAGEPIXMAPUNION_H_

#include <QImage>
#include <QPixmap>

class ImagePixmapUnion {
  // Member-wise copying is OK.
 public:
  ImagePixmapUnion() = default;

  ImagePixmapUnion(const QImage& image) : m_image(image) {}

  ImagePixmapUnion(const QPixmap& pixmap) : m_pixmap(pixmap) {}

  const QImage& image() const { return m_image; }

  const QPixmap& pixmap() const { return m_pixmap; }

  bool isNull() const { return m_image.isNull() && m_pixmap.isNull(); }

 private:
  QImage m_image;
  QPixmap m_pixmap;
};


#endif  // ifndef SCANTAILOR_CORE_IMAGEPIXMAPUNION_H_
