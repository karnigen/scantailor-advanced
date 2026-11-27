// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <QLineF>
#include <QRectF>

/**
 * If \p line (not line segment!) intersects with \p rect,
 * writes intersection points as the new \p line endpoints
 * and returns true.  Otherwise returns false and leaves
 * \p line unmodified.
 */
bool lineBoundedByRect(QLineF& line, const QRectF& rect);
