// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <QPointF>
#include <QPolygonF>
#include <QVector>
#include <functional>

class EditableSpline;
class QTransform;
class QDomDocument;
class QDomElement;
class QString;

class SerializableSpline {
 public:
  SerializableSpline(const EditableSpline& spline);

  explicit SerializableSpline(const QDomElement& el);

  explicit SerializableSpline(const QPolygonF& polygon);

  QDomElement toXml(QDomDocument& doc, const QString& name) const;

  SerializableSpline transformed(const QTransform& xform) const;

  SerializableSpline transformed(const std::function<QPointF(const QPointF&)>& xform) const;

  QPolygonF toPolygon() const { return QPolygonF(m_points); }

 private:
  QVector<QPointF> m_points;
};
