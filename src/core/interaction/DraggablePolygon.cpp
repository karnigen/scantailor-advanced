// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#include "DraggablePolygon.h"

#include <QPainter>

DraggablePolygon::DraggablePolygon() : m_proximityPriority(0) {}

int DraggablePolygon::proximityPriority() const {
  return m_proximityPriority;
}

Proximity DraggablePolygon::proximity(const QPointF& mousePos) {
  double value = polygonPosition().containsPoint(mousePos, Qt::WindingFill) ? 0 : std::numeric_limits<double>::max();
  return Proximity::fromSqDist(value);
}

void DraggablePolygon::dragInitiated(const QPointF& mousePos) {
  m_initialMousePos = mousePos;
  m_initialPolygonPos = polygonPosition();
}

void DraggablePolygon::dragContinuation(const QPointF& mousePos, Qt::KeyboardModifiers mask) {
  polygonMoveRequest(m_initialPolygonPos.translated(mousePos - m_initialMousePos));
}
