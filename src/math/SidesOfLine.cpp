// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#include "SidesOfLine.h"

qreal sidesOfLine(const QLineF& line, const QPointF& p1, const QPointF& p2) {
  const QPointF normal(line.normalVector().p2() - line.p1());
  const QPointF vec1(p1 - line.p1());
  const QPointF vec2(p2 - line.p1());
  const qreal dot1 = normal.x() * vec1.x() + normal.y() * vec1.y();
  const qreal dot2 = normal.x() * vec2.x() + normal.y() * vec2.y();
  return dot1 * dot2;
}
