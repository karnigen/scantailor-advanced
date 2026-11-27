// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#ifndef SCANTAILOR_CORE_CONTENTBOXCOLLECTOR_H_
#define SCANTAILOR_CORE_CONTENTBOXCOLLECTOR_H_

#include "AbstractFilterDataCollector.h"

class ImageTransformation;
class QRectF;

class ContentBoxCollector : public AbstractFilterDataCollector {
 public:
  virtual void process(const ImageTransformation& xform,
                       const QRectF& pageRect,
                       const QRectF& contentRect,
                       bool pageBoundaryEnabled) = 0;
};


#endif
