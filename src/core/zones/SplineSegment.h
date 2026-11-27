// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <QLineF>
#include <QPointF>

#include "SplineVertex.h"

class SplineSegment {
 public:
  SplineVertex::Ptr prev;
  SplineVertex::Ptr next;

  SplineSegment() = default;

  SplineSegment(const SplineVertex::Ptr& prev, const SplineVertex::Ptr& next);

  SplineVertex::Ptr splitAt(const QPointF& pt);

  bool isValid() const;

  bool operator==(const SplineSegment& other) const { return prev == other.prev && next == other.next; }

  QLineF toLine() const { return QLineF(prev->point(), next->point()); }
};
