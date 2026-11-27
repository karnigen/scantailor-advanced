// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <QRectF>
#include <QSizeF>

#include "Alignment.h"
#include "Margins.h"

class QDomDocument;
class QDomElement;
class QString;

namespace page_layout {
class Params {
  // Member-wise copying is OK.
 public:
  Params(const Margins& hardMarginsMm,
         const QRectF& pageRect,
         const QRectF& contentRect,
         const QSizeF& contentSizeMm,
         const Alignment& alignment,
         bool autoMargins,
         bool pageBoundaryEnabled = false,
         const QSizeF& pageBoundarySizeMm = QSizeF());

  explicit Params(const QDomElement& el);

  const Margins& hardMarginsMM() const;

  const QRectF& contentRect() const;

  const QRectF& pageRect() const;

  const QSizeF& contentSizeMM() const;

  const Alignment& alignment() const;

  bool isAutoMarginsEnabled() const;

  bool isPageBoundaryEnabled() const;

  const QSizeF& pageBoundarySizeMM() const;

  QDomElement toXml(QDomDocument& doc, const QString& name) const;

 private:
  Margins m_hardMarginsMM;
  QRectF m_contentRect;
  QRectF m_pageRect;
  QSizeF m_contentSizeMM;
  Alignment m_alignment;
  bool m_autoMargins;
  bool m_pageBoundaryEnabled;
  QSizeF m_pageBoundarySizeMM;
};


inline const Margins& Params::hardMarginsMM() const {
  return m_hardMarginsMM;
}

inline const QRectF& Params::contentRect() const {
  return m_contentRect;
}

inline const QRectF& Params::pageRect() const {
  return m_pageRect;
}

inline const QSizeF& Params::contentSizeMM() const {
  return m_contentSizeMM;
}

inline const Alignment& Params::alignment() const {
  return m_alignment;
}

inline bool Params::isAutoMarginsEnabled() const {
  return m_autoMargins;
}

inline bool Params::isPageBoundaryEnabled() const {
  return m_pageBoundaryEnabled;
}

inline const QSizeF& Params::pageBoundarySizeMM() const {
  return m_pageBoundarySizeMM;
}
}  // namespace page_layout
