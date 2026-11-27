// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <QDateTime>
#include <QPoint>

#include "InteractionHandler.h"

class DragHandler;
class InteractionState;
class QMouseEvent;

/**
 * Objects of this class are intended to be used as a descendants followers
 * of DragHandler objects.  The idea is to collect some statistics about
 * an ongoing drag and decide if the drag was significant or not, in which
 * case we could perform some other operation on mouse release.
 */
class DragWatcher : public InteractionHandler {
 public:
  explicit DragWatcher(DragHandler& dragHandler);

  bool haveSignificantDrag() const;

 protected:
  void onMousePressEvent(QMouseEvent* event, InteractionState& interaction) override;

  void onMouseMoveEvent(QMouseEvent* event, InteractionState& interaction) override;

 private:
  void updateState(QPoint mousePos);

  DragHandler& m_dragHandler;
  QDateTime m_dragStartTime;
  QPoint m_dragStartPos;
  int m_dragMaxSqDist;
  bool m_dragInProgress;
};
