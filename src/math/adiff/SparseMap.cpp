// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#include "SparseMap.h"

namespace adiff {
const size_t SparseMap<2>::ZERO_ELEMENT = ~size_t(0);

SparseMap<2>::SparseMap(size_t numVars)
    : m_numVars(numVars), m_numNonZeroElements(0), m_map(numVars, numVars, ZERO_ELEMENT) {}

void SparseMap<2>::markNonZero(size_t i, size_t j) {
  size_t& el = m_map(i, j);
  if (el == ZERO_ELEMENT) {
    el = m_numNonZeroElements;
    ++m_numNonZeroElements;
  }
}

void SparseMap<2>::markAllNonZero() {
  for (size_t i = 0; i < m_numVars; ++i) {
    for (size_t j = 0; j < m_numVars; ++j) {
      markNonZero(i, j);
    }
  }
}

size_t SparseMap<2>::nonZeroElementIdx(size_t i, size_t j) const {
  return m_map(i, j);
}
}  // namespace adiff