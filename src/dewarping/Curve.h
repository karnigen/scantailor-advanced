// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#ifndef SCANTAILOR_DEWARPING_CURVE_H_
#define SCANTAILOR_DEWARPING_CURVE_H_

#include <QPointF>
#include <vector>

#include "XSpline.h"

class QDomDocument;
class QDomElement;
class QString;

namespace dewarping {
class Curve {
 public:
  Curve();

  explicit Curve(const std::vector<QPointF>& polyline);

  explicit Curve(const XSpline& xspline);

  explicit Curve(const QDomElement& el);

  QDomElement toXml(QDomDocument& doc, const QString& name) const;

  bool isValid() const;

  bool matches(const Curve& other) const;

  const XSpline& xspline() const { return m_xspline; }

  const std::vector<QPointF>& polyline() const { return m_polyline; }

  static bool splineHasLoops(const XSpline& spline);

 private:
  struct CloseEnough;

  static std::vector<QPointF> deserializePolyline(const QDomElement& el);

  static QDomElement serializePolyline(const std::vector<QPointF>& polyline, QDomDocument& doc, const QString& name);

  static XSpline deserializeXSpline(const QDomElement& el);

  static QDomElement serializeXSpline(const XSpline& xspline, QDomDocument& doc, const QString& name);

  static bool approxPolylineMatch(const std::vector<QPointF>& polyline1, const std::vector<QPointF>& polyline2);

  XSpline m_xspline;
  std::vector<QPointF> m_polyline;
};
}  // namespace dewarping
#endif  // ifndef SCANTAILOR_DEWARPING_CURVE_H_
