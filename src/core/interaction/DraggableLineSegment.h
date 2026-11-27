// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <QLineF>
#include <QPointF>
#include <functional>

#include "DraggableObject.h"

class ObjectDragHandler;

class DraggableLineSegment : public DraggableObject {
 public:
  using PositionCallback = std::function<QLineF()>;

  using MoveRequestCallback = std::function<void(const QLineF& line, Qt::KeyboardModifiers mask)>;

  DraggableLineSegment();

  void setProximityPriority(int priority) { m_proximityPriority = priority; }

  int proximityPriority() const override;

  Proximity proximity(const QPointF& mousePos) override;

  void dragInitiated(const QPointF& mousePos) override;

  void dragContinuation(const QPointF& mousePos, Qt::KeyboardModifiers mask) override;

  void setPositionCallback(const PositionCallback& callback) { m_positionCallback = callback; }

  void setMoveRequestCallback(const MoveRequestCallback& callback) { m_moveRequestCallback = callback; }

 protected:
  virtual QLineF lineSegmentPosition() const { return m_positionCallback(); }

  virtual void lineSegmentMoveRequest(const QLineF& line, Qt::KeyboardModifiers mask) {
    m_moveRequestCallback(line, mask);
  }

 private:
  PositionCallback m_positionCallback;
  MoveRequestCallback m_moveRequestCallback;
  QPointF m_initialMousePos;
  QLineF m_initialLinePos;
  int m_proximityPriority;
};
