// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#ifndef SCANTAILOR_FOUNDATION_FLAGOPS_H_
#define SCANTAILOR_FOUNDATION_FLAGOPS_H_

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#define DEFINE_FLAG_OPS(type)                    \
  inline type operator&(type lhs, type rhs) {    \
    return type(unsigned(lhs) & unsigned(rhs));  \
  }                                              \
                                                 \
  inline type operator|(type lhs, type rhs) {    \
    return type(unsigned(lhs) | unsigned(rhs));  \
  }                                              \
                                                 \
  inline type operator^(type lhs, type rhs) {    \
    return type(unsigned(lhs) ^ unsigned(rhs));  \
  }                                              \
                                                 \
  inline type operator~(type val) {              \
    return type(~unsigned(val));                 \
  }                                              \
                                                 \
  inline type& operator&=(type& lhs, type rhs) { \
    lhs = lhs & rhs;                             \
    return lhs;                                  \
  }                                              \
                                                 \
  inline type& operator|=(type& lhs, type rhs) { \
    lhs = lhs | rhs;                             \
    return lhs;                                  \
  }                                              \
                                                 \
  inline type& operator^=(type& lhs, type rhs) { \
    lhs = lhs ^ rhs;                             \
    return lhs;                                  \
  }

#endif  // ifndef SCANTAILOR_FOUNDATION_FLAGOPS_H_
