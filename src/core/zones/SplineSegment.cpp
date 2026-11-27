// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#include "SplineSegment.h"

#include <cassert>

SplineSegment::SplineSegment(const SplineVertex::Ptr& prev, const SplineVertex::Ptr& next) : prev(prev), next(next) {}

SplineVertex::Ptr SplineSegment::splitAt(const QPointF& pt) {
  assert(isValid());
  return prev->insertAfter(pt);
}

bool SplineSegment::isValid() const {
  return prev && next && prev->next(SplineVertex::LOOP_IF_BRIDGED) == next;
}
