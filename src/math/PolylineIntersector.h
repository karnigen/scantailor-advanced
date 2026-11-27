// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <QLineF>
#include <QPointF>
#include <vector>

#include "VecNT.h"

class PolylineIntersector {
 public:
  class Hint {
    friend class PolylineIntersector;

   public:
    Hint();

   private:
    void update(int newSegment);

    int m_lastSegment;
    int m_direction;
  };


  explicit PolylineIntersector(const std::vector<QPointF>& polyline);

  QPointF intersect(const QLineF& line, Hint& hint) const;

 private:
  bool intersectsSegment(const QLineF& normal, int segment) const;

  bool intersectsSpan(const QLineF& normal, const QLineF& span) const;

  QPointF intersectWithSegment(const QLineF& line, int segment) const;

  bool tryIntersectingOutsideOfPolyline(const QLineF& line, QPointF& intersection, Hint& hint) const;

  std::vector<QPointF> m_polyline;
  int m_numSegments;
};
