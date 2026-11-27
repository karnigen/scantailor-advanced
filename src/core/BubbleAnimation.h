// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <QRectF>

class QColor;
class QPaintDevice;
class QPainter;

/**
 * \brief Renders a sequence of frames with a circle of bubles of
 *        varying colors.
 */
class BubbleAnimation {
 public:
  explicit BubbleAnimation(int numBubbles);

  /**
   * \brief Renders the next frame of the animation.
   *
   * \param headColor The color of the head of the string of bubbles.
   * \param tailColor The color of the tail of the string of bubbles.
   * \param pd The device to paint to.
   * \param rect The rectangle in device coordinates to render to.
   *        A null rectangle indicates the whole device area
   *        is to be used.
   * \return Whether more frames follow.  After returning false,
   *         the next call will render the first frame again.
   */
  bool nextFrame(const QColor& headColor, const QColor& tailColor, QPaintDevice* pd, QRectF rect = QRectF());

  /**
   * \brief Renders the next frame of the animation.
   *
   * \param headColor The color of the head of the string of bubbles.
   * \param tailColor The color of the tail of the string of bubbles.
   * \param painter The painter to use for drawing.
   *        Saving and restoring its state is the responsibility
   *        of the caller.
   * \param rect The rectangle in painter coordinates to render to.
   * \return Whether more frames follow.  After returning false,
   *         the next call will render the first frame again.
   */
  bool nextFrame(const QColor& headColor, const QColor& tailColor, QPainter* painter, QRectF rect);

 private:
  int m_numBubbles;
  int m_curFrame;
};
