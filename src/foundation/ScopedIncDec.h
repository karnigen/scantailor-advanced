// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#ifndef SCANTAILOR_FOUNDATION_SCOPEDINCDEC_H_
#define SCANTAILOR_FOUNDATION_SCOPEDINCDEC_H_

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

template <class T>
class ScopedIncDec {
 public:
  explicit ScopedIncDec(T& counter) : m_counter(counter) { ++counter; }

  ~ScopedIncDec() { --m_counter; }

 private:
  T& m_counter;
};


#endif
