// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#ifndef SCANTAILOR_CORE_PAYLOADEVENT_H_
#define SCANTAILOR_CORE_PAYLOADEVENT_H_

#include <QEvent>

template <typename T>
class PayloadEvent : public QEvent {
 public:
  explicit PayloadEvent(const T& payload) : QEvent(User), m_payload(payload) {}

  const T& payload() const { return m_payload; }

  T& payload() { return m_payload; }

 private:
  T m_payload;
};


#endif
