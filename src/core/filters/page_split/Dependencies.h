// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <QSize>

#include "LayoutType.h"
#include "OrthogonalRotation.h"

class QString;
class QDomDocument;
class QDomElement;

namespace page_split {
class Params;

/**
 * \brief Dependencies of a page parameters.
 *
 * Once dependencies change, the stored page parameters are no longer valid.
 */
class Dependencies {
  // Member-wise copying is OK.
 public:
  Dependencies();

  explicit Dependencies(const QDomElement& el);

  Dependencies(const QSize& imageSize, OrthogonalRotation rotation, LayoutType layoutType);

  void setLayoutType(LayoutType type);

  const OrthogonalRotation& orientation() const;

  bool compatibleWith(const Params& params) const;

  bool isNull() const;

  QDomElement toXml(QDomDocument& doc, const QString& tagName) const;

 private:
  QSize m_imageSize;
  OrthogonalRotation m_rotation;
  LayoutType m_layoutType;
};


inline const OrthogonalRotation& Dependencies::orientation() const {
  return m_rotation;
}

inline void Dependencies::setLayoutType(LayoutType type) {
  m_layoutType = type;
}
}  // namespace page_split
