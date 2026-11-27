// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#include "LinearSolver.h"

LinearSolver::LinearSolver(size_t rows_AB, size_t cols_A_rows_X, size_t cols_BX)
    : m_rowsAB(rows_AB), m_colsArowsX(cols_A_rows_X), m_colsBX(cols_BX) {
  if (m_rowsAB < m_colsArowsX) {
    throw std::runtime_error("LinearSolver: can's solve underdetermined systems");
  }
}
