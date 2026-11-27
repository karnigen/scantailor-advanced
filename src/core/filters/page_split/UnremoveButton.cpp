// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#include "UnremoveButton.h"

#include <core/IconProvider.h>

#include <QMouseEvent>
#include <QPainter>

namespace page_split {
UnremoveButton::UnremoveButton(const PositionGetter& positionGetter)
    : m_positionGetter(positionGetter),
      m_clickCallback(&UnremoveButton::noOp),
      m_defaultPixmap(IconProvider::getInstance().getIcon("trashed").pixmap(128, 128)),
      m_hoveredPixmap(IconProvider::getInstance().getIcon("untrash").pixmap(128, 128)),
      m_wasHovered(false) {
  m_proximityInteraction.setProximityCursor(Qt::PointingHandCursor);
  m_proximityInteraction.setProximityStatusTip(tr("Restore removed page."));
}

void UnremoveButton::onPaint(QPainter& painter, const InteractionState& interaction) {
  const QPixmap& pixmap = interaction.proximityLeader(m_proximityInteraction) ? m_hoveredPixmap : m_defaultPixmap;

  QRectF rect(pixmap.rect());
  rect.moveCenter(m_positionGetter());

  painter.setWorldTransform(QTransform());
  painter.drawPixmap(rect.topLeft(), pixmap);
}

void UnremoveButton::onProximityUpdate(const QPointF& screenMousePos, InteractionState& interaction) {
  QRectF rect(m_defaultPixmap.rect());
  rect.moveCenter(m_positionGetter());

  const bool hovered = rect.contains(screenMousePos);
  if (hovered != m_wasHovered) {
    m_wasHovered = hovered;
    interaction.setRedrawRequested(true);
  }

  interaction.updateProximity(m_proximityInteraction, Proximity::fromSqDist(hovered ? 0.0 : 1e10));
}

void UnremoveButton::onMousePressEvent(QMouseEvent* event, InteractionState& interaction) {
  if (!interaction.captured() && interaction.proximityLeader(m_proximityInteraction)) {
    event->accept();
    m_clickCallback();
  }
}

void UnremoveButton::setClickCallback(const UnremoveButton::ClickCallback& callback) {
  m_clickCallback = callback;
}

void UnremoveButton::noOp() {}
}  // namespace page_split