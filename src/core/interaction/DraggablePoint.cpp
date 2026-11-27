// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#include "DraggablePoint.h"

DraggablePoint::DraggablePoint() : m_hitAreaRadius(), m_proximityPriority(1) {}

Proximity DraggablePoint::proximityThreshold(const InteractionState& state) const {
  if (m_hitAreaRadius == 0.0) {
    return state.proximityThreshold();
  } else {
    return Proximity::fromDist(m_hitAreaRadius);
  }
}

int DraggablePoint::proximityPriority() const {
  return m_proximityPriority;
}

Proximity DraggablePoint::proximity(const QPointF& mousePos) {
  return Proximity(pointPosition(), mousePos);
}

void DraggablePoint::dragInitiated(const QPointF& mousePos) {
  m_pointRelativeToMouse = pointPosition() - mousePos;
}

void DraggablePoint::dragContinuation(const QPointF& mousePos, Qt::KeyboardModifiers mask) {
  pointMoveRequest(mousePos + m_pointRelativeToMouse, mask);
}
