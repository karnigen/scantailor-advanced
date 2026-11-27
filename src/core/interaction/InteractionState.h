// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <QCursor>
#include <QString>

#include "NonCopyable.h"
#include "Proximity.h"

class Proximity;

class InteractionState {
  DECLARE_NON_COPYABLE(InteractionState)

 public:
  class Captor;

 private:
  class CaptorList {
   public:
    bool empty() const { return first == nullptr; }

    Captor* first = nullptr;
    Captor* last = nullptr;
  };

 public:
  class Captor {
    friend class InteractionState;

   private:
    struct CopyHelper {
      Captor* captor;

      explicit CopyHelper(Captor* cap) : captor(cap) {}
    };

   public:
    Captor() = default;

    Captor(Captor& other) { swapNodes(other); }

    explicit Captor(CopyHelper other) { swapNodes(*other.captor); }

    ~Captor();

    Captor& operator=(Captor& other);

    Captor& operator=(CopyHelper other);

    explicit operator CopyHelper() { return CopyHelper(this); }

    void release() { unlink(); }

    void unlink();

    bool is_linked() const { return m_list != nullptr; }

    const QCursor& proximityCursor() const { return m_proximityCursor; }

    void setProximityCursor(const QCursor& cursor) { m_proximityCursor = cursor; }

    const QCursor& interactionCursor() const { return m_interactionCursor; }

    void setInteractionCursor(const QCursor& cursor) { m_interactionCursor = cursor; }

    const QString& proximityStatusTip() const { return m_proximityStatusTip; }

    void setProximityStatusTip(const QString& tip) { m_proximityStatusTip = tip; }

    const QString& interactionStatusTip() const { return m_interactionStatusTip; }

    void setInteractionStatusTip(const QString& tip) { m_interactionStatusTip = tip; }

    const QString& interactionOrProximityStatusTip() const {
      return m_interactionStatusTip.isNull() ? m_proximityStatusTip : m_interactionStatusTip;
    }

   private:
    QCursor m_proximityCursor;
    QCursor m_interactionCursor;
    QString m_proximityStatusTip;
    QString m_interactionStatusTip;
    Captor* m_previous = nullptr;
    Captor* m_next = nullptr;
    CaptorList* m_list = nullptr;

    void swapNodes(Captor& other);
  };


  InteractionState();

  ~InteractionState();

  void capture(Captor& captor);

  bool captured() const { return !m_captorList.empty(); }

  bool capturedBy(const Captor& captor) const;

  void resetProximity();

  void updateProximity(Captor& captor,
                       const Proximity& proximity,
                       int priority = 0,
                       Proximity proximityThreshold = Proximity());

  bool proximityLeader(const Captor& captor) const;

  const Proximity& proximityThreshold() const { return m_proximityThreshold; }

  QCursor cursor() const;

  QString statusTip() const;

  const QString& defaultStatusTip() const { return m_defaultStatusTip; }

  void setDefaultStatusTip(const QString& statusTip) { m_defaultStatusTip = statusTip; }

  bool redrawRequested() const { return m_redrawRequested; }

  void setRedrawRequested(bool requested) { m_redrawRequested = requested; }

 private:
  /**
   * Returns true if the provided proximity is better than the stored one.
   */
  bool betterProximity(const Proximity& proximity, int priority) const;

  static void append(CaptorList& list, Captor& captor);

  static void unlinkAll(CaptorList& list);

  QString m_defaultStatusTip;
  CaptorList m_captorList;
  CaptorList m_proximityLeader;
  Proximity m_bestProximity;
  Proximity m_proximityThreshold;
  int m_bestProximityPriority;
  bool m_redrawRequested;
};
