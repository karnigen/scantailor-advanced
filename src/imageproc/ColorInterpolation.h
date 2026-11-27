// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <QColor>

namespace imageproc {
/**
 * \brief Returns a color between the provided two.
 *
 * Returns a color between \p from and \p to according to \p dist.
 * \p dist 0 corresponds to \p from, while \p dist 1 corresponds to \p to.
 */
QColor colorInterpolation(const QColor& from, const QColor& to, double dist);
}  // namespace imageproc
