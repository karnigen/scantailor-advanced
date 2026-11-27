// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <QPolygonF>

#include <optional>

#include "OrthogonalRotation.h"

class QDomDocument;
class QDomElement;
class QString;

namespace deskew {
/**
 * \brief Dependencies of deskew parameters.
 *
 * Once dependencies change, deskew parameters are no longer valid.
 */
class Dependencies {
 public:
  // Member-wise copying is OK.

  Dependencies();

  Dependencies(const QPolygonF& pageOutline, OrthogonalRotation rotation);

  Dependencies(const QPolygonF& pageOutline, OrthogonalRotation rotation, bool algoContentBased);

  explicit Dependencies(const QDomElement& depsEl);

  ~Dependencies();

  bool matches(const Dependencies& other) const;

  QDomElement toXml(QDomDocument& doc, const QString& name) const;

 private:
  QPolygonF m_pageOutline;
  OrthogonalRotation m_rotation;
  std::optional<bool> m_algoContentBased;
};
}  // namespace deskew
