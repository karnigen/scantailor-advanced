// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <limits>

namespace spfit {
class OptimizationResult {
 public:
  OptimizationResult(double forceBefore, double forceAfter);

  double forceBefore() const { return m_forceBefore; }

  double forceAfter() const { return m_forceAfter; }

  /**
   * \brief Returns force decrease in percents.
   *
   * Force decrease can theoretically be negative.
   *
   * \note Improvements from different optimization runs can't be compared,
   *       as the absolute force values depend on the number of samples,
   *       which varies from one optimization iteration to another.
   */
  double improvementPercentage() const;

 private:
  double m_forceBefore;
  double m_forceAfter;
};
}  // namespace spfit
