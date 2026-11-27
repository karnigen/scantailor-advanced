// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#ifndef SCANTAILOR_INTERACTION_INTERACTIONHANDLER_H_
#define SCANTAILOR_INTERACTION_INTERACTIONHANDLER_H_

#include <list>
#include <memory>

#include "NonCopyable.h"

class InteractionState;
class QPainter;
class QKeyEvent;
class QMouseEvent;
class QWheelEvent;
class QContextMenuEvent;
class QPointF;

class InteractionHandler {
  DECLARE_NON_COPYABLE(InteractionHandler)

 public:
  InteractionHandler();

  virtual ~InteractionHandler();

  bool is_linked() const { return m_parentList != nullptr; }

  void unlink();

  void paint(QPainter& painter, const InteractionState& interaction);

  void proximityUpdate(const QPointF& screenMousePos, InteractionState& interaction);

  void keyPressEvent(QKeyEvent* event, InteractionState& interaction);

  void keyReleaseEvent(QKeyEvent* event, InteractionState& interaction);

  void mousePressEvent(QMouseEvent* event, InteractionState& interaction);

  void mouseReleaseEvent(QMouseEvent* event, InteractionState& interaction);

  void mouseDoubleClickEvent(QMouseEvent* event, InteractionState& interaction);

  void mouseMoveEvent(QMouseEvent* event, InteractionState& interaction);

  void wheelEvent(QWheelEvent* event, InteractionState& interaction);

  void contextMenuEvent(QContextMenuEvent* event, InteractionState& interaction);

  void makePeerPreceeder(InteractionHandler& handler);

  void makePeerFollower(InteractionHandler& handler);

  void makeFirstPreceeder(InteractionHandler& handler);

  void makeLastPreceeder(InteractionHandler& handler);

  void makeFirstFollower(InteractionHandler& handler);

  void makeLastFollower(InteractionHandler& handler);

 protected:
  virtual void onPaint(QPainter& painter, const InteractionState& interaction) {}

  virtual void onProximityUpdate(const QPointF& screenMousePos, InteractionState& interaction) {}

  virtual void onKeyPressEvent(QKeyEvent* event, InteractionState& interaction) {}

  virtual void onKeyReleaseEvent(QKeyEvent* event, InteractionState& interaction) {}

  virtual void onMousePressEvent(QMouseEvent* event, InteractionState& interaction) {}

  virtual void onMouseReleaseEvent(QMouseEvent* event, InteractionState& interaction) {}

  virtual void onMouseDoubleClickEvent(QMouseEvent* event, InteractionState& interaction) {}

  virtual void onMouseMoveEvent(QMouseEvent* event, InteractionState& interaction) {}

  virtual void onWheelEvent(QWheelEvent* event, InteractionState& interaction) {}

  virtual void onContextMenuEvent(QContextMenuEvent* event, InteractionState& interaction) {}

  static bool defaultInteractionPermitter(const InteractionState& interaction);

 private:
  using HandlerList = std::list<InteractionHandler*>;

  void insertInto(HandlerList& list, HandlerList::iterator position);

  void clearAndDispose(HandlerList& list);

  HandlerList* m_parentList = nullptr;
  HandlerList::iterator m_parentIterator;
  std::shared_ptr<HandlerList> m_preceeders;
  std::shared_ptr<HandlerList> m_followers;
};


#endif  // ifndef SCANTAILOR_INTERACTION_INTERACTIONHANDLER_H_
