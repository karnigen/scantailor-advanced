// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#include "InteractionHandler.h"

#include <QKeyEvent>
#include <QPainter>

#include "InteractionState.h"

#define DISPATCH(list, call)               \
  {                                        \
    auto it = list->begin();               \
    const auto end = list->end();          \
    while (it != end) {                    \
      InteractionHandler* handler = *it++; \
      handler->call;                       \
    }                                      \
  }

#define RETURN_IF_ACCEPTED(event) \
  {                               \
    if (event->isAccepted()) {    \
      return;                     \
    }                             \
  }

namespace {
class ScopedClearAcceptance {
  DECLARE_NON_COPYABLE(ScopedClearAcceptance)

 public:
  explicit ScopedClearAcceptance(QEvent* event);

  ~ScopedClearAcceptance();

 private:
  QEvent* m_event;
  bool m_wasAccepted;
};


ScopedClearAcceptance::ScopedClearAcceptance(QEvent* event) : m_event(event), m_wasAccepted(event->isAccepted()) {
  m_event->setAccepted(false);
}

ScopedClearAcceptance::~ScopedClearAcceptance() {
  if (m_wasAccepted) {
    m_event->setAccepted(true);
  }
}
}  // anonymous namespace

InteractionHandler::InteractionHandler()
    : m_preceeders(std::make_shared<HandlerList>()), m_followers(std::make_shared<HandlerList>()) {}

InteractionHandler::~InteractionHandler() {
  clearAndDispose(*m_preceeders);
  clearAndDispose(*m_followers);
  unlink();
}

void InteractionHandler::unlink() {
  if (m_parentList) {
    m_parentList->erase(m_parentIterator);
    m_parentList = nullptr;
  }
}

void InteractionHandler::insertInto(HandlerList& list, HandlerList::iterator position) {
  m_parentIterator = list.insert(position, this);
  m_parentList = &list;
}

void InteractionHandler::clearAndDispose(HandlerList& list) {
  while (!list.empty()) {
    InteractionHandler* handler = list.front();
    handler->unlink();
    delete handler;
  }
}

void InteractionHandler::paint(QPainter& painter, const InteractionState& interaction) {
  // Keep them alive in case this object gets destroyed.
  std::shared_ptr<HandlerList> preceeders(m_preceeders);
  std::shared_ptr<HandlerList> followers(m_followers);

  DISPATCH(preceeders, paint(painter, interaction));
  painter.save();
  onPaint(painter, interaction);
  painter.restore();
  DISPATCH(followers, paint(painter, interaction));
}

void InteractionHandler::proximityUpdate(const QPointF& screenMousePos, InteractionState& interaction) {
  // Keep them alive in case this object gets destroyed.
  std::shared_ptr<HandlerList> preceeders(m_preceeders);
  std::shared_ptr<HandlerList> followers(m_followers);

  DISPATCH(preceeders, proximityUpdate(screenMousePos, interaction));
  onProximityUpdate(screenMousePos, interaction);
  assert(!interaction.captured() && "onProximityUpdate() must not capture interaction");
  DISPATCH(followers, proximityUpdate(screenMousePos, interaction));
}

void InteractionHandler::keyPressEvent(QKeyEvent* event, InteractionState& interaction) {
  RETURN_IF_ACCEPTED(event);

  // Keep them alive in case this object gets destroyed.
  std::shared_ptr<HandlerList> preceeders(m_preceeders);
  std::shared_ptr<HandlerList> followers(m_followers);

  DISPATCH(preceeders, keyPressEvent(event, interaction));
  RETURN_IF_ACCEPTED(event);
  onKeyPressEvent(event, interaction);
  ScopedClearAcceptance guard(event);
  DISPATCH(followers, keyPressEvent(event, interaction));
}

void InteractionHandler::keyReleaseEvent(QKeyEvent* event, InteractionState& interaction) {
  RETURN_IF_ACCEPTED(event);
  // Keep them alive in case this object gets destroyed.
  std::shared_ptr<HandlerList> preceeders(m_preceeders);
  std::shared_ptr<HandlerList> followers(m_followers);

  DISPATCH(preceeders, keyReleaseEvent(event, interaction));
  RETURN_IF_ACCEPTED(event);
  onKeyReleaseEvent(event, interaction);
  ScopedClearAcceptance guard(event);
  DISPATCH(followers, keyReleaseEvent(event, interaction));
}

void InteractionHandler::mousePressEvent(QMouseEvent* event, InteractionState& interaction) {
  RETURN_IF_ACCEPTED(event);

  // Keep them alive in case this object gets destroyed.
  std::shared_ptr<HandlerList> preceeders(m_preceeders);
  std::shared_ptr<HandlerList> followers(m_followers);

  DISPATCH(preceeders, mousePressEvent(event, interaction));
  RETURN_IF_ACCEPTED(event);
  onMousePressEvent(event, interaction);
  ScopedClearAcceptance guard(event);
  DISPATCH(followers, mousePressEvent(event, interaction));
}

void InteractionHandler::mouseReleaseEvent(QMouseEvent* event, InteractionState& interaction) {
  RETURN_IF_ACCEPTED(event);
  // Keep them alive in case this object gets destroyed.
  std::shared_ptr<HandlerList> preceeders(m_preceeders);
  std::shared_ptr<HandlerList> followers(m_followers);

  DISPATCH(preceeders, mouseReleaseEvent(event, interaction));
  RETURN_IF_ACCEPTED(event);
  onMouseReleaseEvent(event, interaction);
  ScopedClearAcceptance guard(event);
  DISPATCH(followers, mouseReleaseEvent(event, interaction));
}

void InteractionHandler::mouseDoubleClickEvent(QMouseEvent* event, InteractionState& interaction) {
  RETURN_IF_ACCEPTED(event);
  // Keep them alive in case this object gets destroyed.
  std::shared_ptr<HandlerList> preceeders(m_preceeders);
  std::shared_ptr<HandlerList> followers(m_followers);

  DISPATCH(preceeders, mouseDoubleClickEvent(event, interaction));
  RETURN_IF_ACCEPTED(event);
  onMouseDoubleClickEvent(event, interaction);
  ScopedClearAcceptance guard(event);
  DISPATCH(followers, mouseDoubleClickEvent(event, interaction));
}

void InteractionHandler::mouseMoveEvent(QMouseEvent* event, InteractionState& interaction) {
  RETURN_IF_ACCEPTED(event);

  // Keep them alive in case this object gets destroyed.
  std::shared_ptr<HandlerList> preceeders(m_preceeders);
  std::shared_ptr<HandlerList> followers(m_followers);

  DISPATCH(preceeders, mouseMoveEvent(event, interaction));
  RETURN_IF_ACCEPTED(event);
  onMouseMoveEvent(event, interaction);
  ScopedClearAcceptance guard(event);
  DISPATCH(followers, mouseMoveEvent(event, interaction));
}

void InteractionHandler::wheelEvent(QWheelEvent* event, InteractionState& interaction) {
  RETURN_IF_ACCEPTED(event);

  // Keep them alive in case this object gets destroyed.
  std::shared_ptr<HandlerList> preceeders(m_preceeders);
  std::shared_ptr<HandlerList> followers(m_followers);

  DISPATCH(preceeders, wheelEvent(event, interaction));
  RETURN_IF_ACCEPTED(event);
  onWheelEvent(event, interaction);
  ScopedClearAcceptance guard(event);
  DISPATCH(followers, wheelEvent(event, interaction));
}

void InteractionHandler::contextMenuEvent(QContextMenuEvent* event, InteractionState& interaction) {
  RETURN_IF_ACCEPTED(event);
  // Keep them alive in case this object gets destroyed.
  std::shared_ptr<HandlerList> preceeders(m_preceeders);
  std::shared_ptr<HandlerList> followers(m_followers);

  DISPATCH(preceeders, contextMenuEvent(event, interaction));
  RETURN_IF_ACCEPTED(event);
  onContextMenuEvent(event, interaction);
  ScopedClearAcceptance guard(event);
  DISPATCH(followers, contextMenuEvent(event, interaction));
}

void InteractionHandler::makePeerPreceeder(InteractionHandler& handler) {
  assert(m_parentList && &handler != this);
  handler.unlink();
  handler.insertInto(*m_parentList, m_parentIterator);
}

void InteractionHandler::makePeerFollower(InteractionHandler& handler) {
  assert(m_parentList && &handler != this);
  handler.unlink();
  auto position = m_parentIterator;
  handler.insertInto(*m_parentList, ++position);
}

void InteractionHandler::makeFirstPreceeder(InteractionHandler& handler) {
  handler.unlink();
  handler.insertInto(*m_preceeders, m_preceeders->begin());
}

void InteractionHandler::makeLastPreceeder(InteractionHandler& handler) {
  handler.unlink();
  handler.insertInto(*m_preceeders, m_preceeders->end());
}

void InteractionHandler::makeFirstFollower(InteractionHandler& handler) {
  handler.unlink();
  handler.insertInto(*m_followers, m_followers->begin());
}

void InteractionHandler::makeLastFollower(InteractionHandler& handler) {
  handler.unlink();
  handler.insertInto(*m_followers, m_followers->end());
}

bool InteractionHandler::defaultInteractionPermitter(const InteractionState& interaction) {
  return !interaction.captured();
}
