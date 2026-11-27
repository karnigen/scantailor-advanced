// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <QPointF>
#include <QPolygonF>
#include <functional>

#include "DraggableObject.h"

class ObjectDragHandler;

class DraggablePolygon : public DraggableObject {
 public:
  using PositionCallback = std::function<QPolygonF()>;

  using MoveRequestCallback = std::function<void(const QPolygonF& polygon)>;

  DraggablePolygon();

  int proximityPriority() const override;

  Proximity proximity(const QPointF& mousePos) override;

  void dragInitiated(const QPointF& mousePos) override;

  void dragContinuation(const QPointF& mousePos, Qt::KeyboardModifiers mask) override;

  void setProximityPriority(int priority) { m_proximityPriority = priority; }

  void setPositionCallback(const PositionCallback& callback) { m_positionCallback = callback; }

  void setMoveRequestCallback(const MoveRequestCallback& callback) { m_moveRequestCallback = callback; }

 protected:
  virtual QPolygonF polygonPosition() const { return m_positionCallback(); }

  virtual void polygonMoveRequest(const QPolygonF& polygon) { m_moveRequestCallback(polygon); }

 private:
  PositionCallback m_positionCallback;
  MoveRequestCallback m_moveRequestCallback;
  QPointF m_initialMousePos;
  QPolygonF m_initialPolygonPos;
  int m_proximityPriority;
};
