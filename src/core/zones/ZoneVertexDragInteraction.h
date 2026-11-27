// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <QCoreApplication>
#include <QPointF>

#include "BasicSplineVisualizer.h"
#include "EditableSpline.h"
#include "InteractionHandler.h"
#include "InteractionState.h"

class ZoneInteractionContext;

class ZoneVertexDragInteraction : public InteractionHandler {
  Q_DECLARE_TR_FUNCTIONS(ZoneVertexDragInteraction)
 public:
  ZoneVertexDragInteraction(ZoneInteractionContext& context,
                            InteractionState& interaction,
                            const EditableSpline::Ptr& spline,
                            const SplineVertex::Ptr& vertex);

 protected:
  void onPaint(QPainter& painter, const InteractionState& interaction) override;

  void onMouseReleaseEvent(QMouseEvent* event, InteractionState& interaction) override;

  void onMouseMoveEvent(QMouseEvent* event, InteractionState& interaction) override;

 private:
  void checkProximity(const InteractionState& interaction);

  ZoneInteractionContext& m_context;
  EditableSpline::Ptr m_spline;
  SplineVertex::Ptr m_vertex;
  InteractionState::Captor m_interaction;
  BasicSplineVisualizer m_visualizer;
  QPointF m_dragOffset;
};
