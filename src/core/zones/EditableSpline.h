// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <QPolygonF>
#include <memory>

#include "SplineSegment.h"
#include "SplineVertex.h"

class SerializableSpline;

class EditableSpline {
 public:
  using Ptr = std::shared_ptr<EditableSpline>;

  class SegmentIterator {
   public:
    explicit SegmentIterator(EditableSpline& spline) : m_nextVertex(spline.firstVertex()) {}

    bool hasNext() const;

    SplineSegment next();

   private:
    SplineVertex::Ptr m_nextVertex;
  };

  EditableSpline();

  EditableSpline(const SerializableSpline& spline);

  virtual ~EditableSpline();

  void appendVertex(const QPointF& pt);

  SplineVertex::Ptr firstVertex() const { return m_sentinel->firstVertex(); }

  SplineVertex::Ptr lastVertex() const { return m_sentinel->lastVertex(); }

  bool hasAtLeastSegments(int num) const;

  bool bridged() const { return m_sentinel->bridged(); }

  void setBridged(bool bridged) { m_sentinel->setBridged(true); }

  QPolygonF toPolygon() const;

 private:
  std::shared_ptr<SentinelSplineVertex> m_sentinel = std::make_shared<SentinelSplineVertex>();
};
