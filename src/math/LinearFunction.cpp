// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#include "LinearFunction.h"

#include <algorithm>

LinearFunction::LinearFunction(size_t numVars) : a(numVars), b(0) {}

void LinearFunction::reset() {
  a.fill(0);
  b = 0;
}

double LinearFunction::evaluate(const double* x) const {
  const size_t numVars = this->numVars();

  double sum = b;
  for (size_t i = 0; i < numVars; ++i) {
    sum += a[i] * x[i];
  }
  return sum;
}

void LinearFunction::swap(LinearFunction& other) {
  a.swap(other.a);
  std::swap(b, other.b);
}

LinearFunction& LinearFunction::operator+=(const LinearFunction& other) {
  a += other.a;
  b += other.b;
  return *this;
}

LinearFunction& LinearFunction::operator*=(double scalar) {
  a *= scalar;
  b *= scalar;
  return *this;
}
