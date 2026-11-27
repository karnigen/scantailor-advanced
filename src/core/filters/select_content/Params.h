// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#ifndef SCANTAILOR_SELECT_CONTENT_PARAMS_H_
#define SCANTAILOR_SELECT_CONTENT_PARAMS_H_

#include <AutoManualMode.h>

#include <QRectF>
#include <QSizeF>
#include <QString>
#include <cmath>

#include "Dependencies.h"
#include "Margins.h"

class QDomDocument;
class QDomElement;
class QString;

namespace select_content {
class Params {
 public:
  // Member-wise copying is OK.

  explicit Params(const Dependencies& deps);

  Params(const QRectF& contentRect,
         const QSizeF& sizeMm,
         const QRectF& pageRect,
         const Dependencies& deps,
         AutoManualMode contentDetectionMode,
         AutoManualMode pageDetectionMode,
         bool fineTuneCorners,
         const QString& pageSizePresetId = QString());

  explicit Params(const QDomElement& filterEl);

  QDomElement toXml(QDomDocument& doc, const QString& name) const;

  ~Params();

  const QRectF& contentRect() const;

  const QRectF& pageRect() const;

  const QSizeF& contentSizeMM() const;

  const Dependencies& dependencies() const;

  AutoManualMode contentDetectionMode() const;

  AutoManualMode pageDetectionMode() const;

  bool isFineTuningEnabled() const;

  const QString& pageSizePresetId() const;

  void setContentDetectionMode(AutoManualMode mode);

  void setPageDetectionMode(AutoManualMode mode);

  void setContentRect(const QRectF& rect);

  void setPageRect(const QRectF& rect);

  void setContentSizeMM(const QSizeF& size);

  void setDependencies(const Dependencies& deps);

  void setFineTuneCornersEnabled(bool fineTuneCorners);

  void setPageSizePresetId(const QString& pageSizePresetId);

 private:
  QRectF m_contentRect;
  QRectF m_pageRect;
  QSizeF m_contentSizeMM;
  Dependencies m_deps;
  AutoManualMode m_contentDetectionMode;
  AutoManualMode m_pageDetectionMode;
  bool m_fineTuneCorners;
  QString m_pageSizePresetId;
};


inline const QRectF& Params::contentRect() const {
  return m_contentRect;
}

inline const QRectF& Params::pageRect() const {
  return m_pageRect;
}

inline const QSizeF& Params::contentSizeMM() const {
  return m_contentSizeMM;
}

inline const Dependencies& Params::dependencies() const {
  return m_deps;
}

inline AutoManualMode Params::contentDetectionMode() const {
  return m_contentDetectionMode;
}

inline AutoManualMode Params::pageDetectionMode() const {
  return m_pageDetectionMode;
}

inline bool Params::isFineTuningEnabled() const {
  return m_fineTuneCorners;
}

inline const QString& Params::pageSizePresetId() const {
  return m_pageSizePresetId;
}

inline void Params::setContentDetectionMode(const AutoManualMode mode) {
  m_contentDetectionMode = mode;
}

inline void Params::setPageDetectionMode(const AutoManualMode mode) {
  m_pageDetectionMode = mode;
}

inline void Params::setContentRect(const QRectF& rect) {
  m_contentRect = rect;
}

inline void Params::setPageRect(const QRectF& rect) {
  m_pageRect = rect;
}

inline void Params::setContentSizeMM(const QSizeF& size) {
  m_contentSizeMM = size;
}

inline void Params::setDependencies(const Dependencies& deps) {
  m_deps = deps;
}

inline void Params::setFineTuneCornersEnabled(bool fineTuneCorners) {
  m_fineTuneCorners = fineTuneCorners;
}

inline void Params::setPageSizePresetId(const QString& pageSizePresetId) {
  m_pageSizePresetId = pageSizePresetId;
}
}  // namespace select_content
#endif  // ifndef SCANTAILOR_SELECT_CONTENT_PARAMS_H_
