// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

class QPainter;
class QPixmap;

class PixmapRenderer {
 public:
  /**
   * \brief Workarounds an issue with QPainter::drawPixmap().
   *
   * This method is more or less equivalent to:
   * \code
   * QPainter::drawPixmap(0, 0, pixmap);
   * \endcode
   * However, Qt's raster paint engine for some reason refuses to draw
   * the pixmap at all if a very strong zoom is applied (as of Qt 5.4.0).
   * This method solves the problem above by calculating the visible area
   * of the pixmap and communicating that information to QPainter.
   */
  static void drawPixmap(QPainter& painter, const QPixmap& pixmap);
};
