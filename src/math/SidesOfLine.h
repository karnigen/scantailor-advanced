// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <QLineF>
#include <QPointF>

/**
 * This function allows you to check if a pair of points is on the same
 * or different sides of a line.
 *
 * Returns:
 * \li Negative value, if points are on different sides of line.
 * \li Positive value, if points are on the same side of line.
 * \li Zero, if one or both of the points are on the line.
 *
 * \note Line's endpoints don't matter - consider the whole line,
 *       not a line segment.  If the line is really a point, zero will
 *       always be returned.
 */
qreal sidesOfLine(const QLineF& line, const QPointF& p1, const QPointF& p2);
