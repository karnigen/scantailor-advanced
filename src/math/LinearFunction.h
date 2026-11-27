// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <cstddef>

#include "VecT.h"

/**
 * A linear function from arbitrary number of variables
 * expressed in matrix form:
 * \code
 * F(x) = a^T * x + b
 * \endcode
 */
class LinearFunction {
  // Member-wise copying is OK.
 public:
  VecT<double> a;
  double b;

  /**
   * Constructs a linear function of the given number of variables,
   * initializing everything to zero.
   */
  explicit LinearFunction(size_t numVars = 0);

  /**
   * Resets everything to zero, so that F(x) = 0
   */
  void reset();

  size_t numVars() const { return a.size(); }

  /**
   * Evaluates a^T * x + b
   */
  double evaluate(const double* x) const;

  void swap(LinearFunction& other);

  LinearFunction& operator+=(const LinearFunction& other);

  LinearFunction& operator*=(double scalar);
};


inline void swap(LinearFunction& f1, LinearFunction& f2) {
  f1.swap(f2);
}
