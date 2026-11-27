// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <QColor>
#include <QPen>

#include "EditableSpline.h"

class EditableZoneSet;
class QPainter;
class QTransform;

class BasicSplineVisualizer {
 public:
  BasicSplineVisualizer();

  QRgb solidColor() const { return m_solidColor; }

  QRgb highlightBrightColor() const { return m_highlightBrightColor; }

  QRgb highlightDarkColor() const { return m_highlightDarkColor; }

  void drawVertex(QPainter& painter, const QPointF& pt, const QColor& color);

  void drawSplines(QPainter& painter, const QTransform& toScreen, const EditableZoneSet& zones);

  virtual void drawSpline(QPainter& painter, const QTransform& toScreen, const EditableSpline::Ptr& spline);

  virtual void prepareForSpline(QPainter& painter, const EditableSpline::Ptr& spline);

 protected:
  QRgb m_solidColor;
  QRgb m_highlightBrightColor;
  QRgb m_highlightDarkColor;
  QPen m_pen;
};
