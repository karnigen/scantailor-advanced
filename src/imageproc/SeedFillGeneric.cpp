// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#include "SeedFillGeneric.h"

namespace imageproc {
namespace detail {
namespace seed_fill_generic {
void initHorTransitions(std::vector<HTransition>& transitions, const int width) {
  transitions.reserve(width);

  if (width == 1) {
    // No transitions allowed.
    transitions.emplace_back(0, 0);
    return;
  }

  // Only east transition is allowed.
  transitions.emplace_back(0, 1);

  for (int i = 1; i < width - 1; ++i) {
    // Both transitions are allowed.
    transitions.emplace_back(-1, 1);
  }

  // Only west transition is allowed.
  transitions.emplace_back(-1, 0);
}

void initVertTransitions(std::vector<VTransition>& transitions, const int height) {
  transitions.reserve(height);

  if (height == 1) {
    // No transitions allowed.
    transitions.emplace_back(0, 0);
    return;
  }

  // Only south transition is allowed.
  transitions.emplace_back(0, ~0);

  for (int i = 1; i < height - 1; ++i) {
    // Both transitions are allowed.
    transitions.emplace_back(~0, ~0);
  }

  // Only north transition is allowed.
  transitions.emplace_back(~0, 0);
}
}  // namespace seed_fill_generic
}  // namespace detail
}  // namespace imageproc