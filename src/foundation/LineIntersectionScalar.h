// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <QLineF>

/**
 * Finds such scalars s1 and s2, so that "line1.pointAt(s1)" and "line2.pointAt(s2)"
 * would be the intersection point between line1 and line2.  Returns false if the
 * lines are parallel or if any of the lines have zero length and therefore no direction.
 */
bool lineIntersectionScalar(const QLineF& line1, const QLineF& line2, double& s1, double& s2);

/**
 * Same as the one above, but doesn't bother to calculate s2.
 */
bool lineIntersectionScalar(const QLineF& line1, const QLineF& line2, double& s1);
