// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <QLineF>

/**
 * \brief Traverses a grid along a line segment.
 *
 * Think about drawing a line on an image.
 */
class GridLineTraverser {
  // Member-wise copying is OK.
 public:
  explicit GridLineTraverser(const QLineF& line);

  bool hasNext() const { return m_stopsDone < m_totalStops; }

  QPoint next();

 private:
  QLineF m_line;
  double m_dt;
  int m_totalStops;
  int m_stopsDone;
};
