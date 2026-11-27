// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#include "InteractionState.h"

InteractionState::Captor& InteractionState::Captor::operator=(Captor& other) {
  swapNodes(other);
  other.unlink();
  return *this;
}

InteractionState::Captor& InteractionState::Captor::operator=(CopyHelper other) {
  return (*this = *other.captor);
}

InteractionState::Captor::~Captor() {
  unlink();
}

void InteractionState::Captor::unlink() {
  if (!m_list) {
    return;
  }

  if (m_previous) {
    m_previous->m_next = m_next;
  } else {
    m_list->first = m_next;
  }
  if (m_next) {
    m_next->m_previous = m_previous;
  } else {
    m_list->last = m_previous;
  }
  m_previous = nullptr;
  m_next = nullptr;
  m_list = nullptr;
}

void InteractionState::Captor::swapNodes(Captor& other) {
  if (this == &other) {
    return;
  }

  if (!m_list) {
    if (other.m_list) {
      other.swapNodes(*this);
    }
    return;
  }
  if (!other.m_list) {
    CaptorList* const list = m_list;
    Captor* const previous = m_previous;
    Captor* const next = m_next;
    if (previous) {
      previous->m_next = &other;
    } else {
      list->first = &other;
    }
    if (next) {
      next->m_previous = &other;
    } else {
      list->last = &other;
    }
    other.m_list = list;
    other.m_previous = previous;
    other.m_next = next;
    m_list = nullptr;
    m_previous = nullptr;
    m_next = nullptr;
    return;
  }

  CaptorList* const thisList = m_list;
  CaptorList* const otherList = other.m_list;
  Captor* const thisPrevious = m_previous;
  Captor* const thisNext = m_next;
  Captor* const otherPrevious = other.m_previous;
  Captor* const otherNext = other.m_next;

  if (thisList == otherList && thisNext == &other) {
    if (thisPrevious) {
      thisPrevious->m_next = &other;
    } else {
      thisList->first = &other;
    }
    other.m_previous = thisPrevious;
    other.m_next = this;
    m_previous = &other;
    m_next = otherNext;
    if (otherNext) {
      otherNext->m_previous = this;
    } else {
      thisList->last = this;
    }
  } else if (thisList == otherList && otherNext == this) {
    if (otherPrevious) {
      otherPrevious->m_next = this;
    } else {
      thisList->first = this;
    }
    m_previous = otherPrevious;
    m_next = &other;
    other.m_previous = this;
    other.m_next = thisNext;
    if (thisNext) {
      thisNext->m_previous = &other;
    } else {
      thisList->last = &other;
    }
  } else {
    if (thisPrevious) {
      thisPrevious->m_next = &other;
    } else {
      thisList->first = &other;
    }
    if (thisNext) {
      thisNext->m_previous = &other;
    } else {
      thisList->last = &other;
    }
    if (otherPrevious) {
      otherPrevious->m_next = this;
    } else {
      otherList->first = this;
    }
    if (otherNext) {
      otherNext->m_previous = this;
    } else {
      otherList->last = this;
    }
    m_list = otherList;
    m_previous = otherPrevious;
    m_next = otherNext;
    other.m_list = thisList;
    other.m_previous = thisPrevious;
    other.m_next = thisNext;
  }
}

InteractionState::InteractionState()
    : m_proximityThreshold(Proximity::fromDist(10.0)),
      m_bestProximityPriority(std::numeric_limits<int>::min()),
      m_redrawRequested(false) {}

InteractionState::~InteractionState() {
  unlinkAll(m_captorList);
  unlinkAll(m_proximityLeader);
}

void InteractionState::capture(Captor& captor) {
  append(m_captorList, captor);
}

bool InteractionState::capturedBy(const Captor& captor) const {
  return m_captorList.last == &captor;
}

void InteractionState::resetProximity() {
  unlinkAll(m_proximityLeader);
  m_bestProximity = Proximity();
  m_bestProximityPriority = std::numeric_limits<int>::min();
}

void InteractionState::updateProximity(Captor& captor,
                                       const Proximity& proximity,
                                       int priority,
                                       Proximity proximityThreshold) {
  if (captor.is_linked()) {
    return;
  }

  if (proximityThreshold == Proximity()) {
    proximityThreshold = m_proximityThreshold;
  }

  if (proximity <= proximityThreshold) {
    if (betterProximity(proximity, priority)) {
      unlinkAll(m_proximityLeader);
      append(m_proximityLeader, captor);
      m_bestProximity = proximity;
      m_bestProximityPriority = priority;
    }
  }
}

bool InteractionState::proximityLeader(const Captor& captor) const {
  return m_proximityLeader.first == &captor;
}

bool InteractionState::betterProximity(const Proximity& proximity, const int priority) const {
  if (priority != m_bestProximityPriority) {
    return priority > m_bestProximityPriority;
  }
  return proximity < m_bestProximity;
}

QCursor InteractionState::cursor() const {
  if (m_captorList.last) {
    return m_captorList.last->interactionCursor();
  } else if (m_proximityLeader.first) {
    return m_proximityLeader.first->proximityCursor();
  } else {
    return QCursor();
  }
}

QString InteractionState::statusTip() const {
  if (m_captorList.last) {
    return m_captorList.last->interactionOrProximityStatusTip();
  } else if (m_proximityLeader.first) {
    return m_proximityLeader.first->proximityStatusTip();
  } else {
    return m_defaultStatusTip;
  }
}

void InteractionState::append(CaptorList& list, Captor& captor) {
  captor.unlink();
  captor.m_list = &list;
  captor.m_previous = list.last;
  captor.m_next = nullptr;
  if (list.last) {
    list.last->m_next = &captor;
  } else {
    list.first = &captor;
  }
  list.last = &captor;
}

void InteractionState::unlinkAll(CaptorList& list) {
  while (list.first) {
    list.first->unlink();
  }
}
