// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#ifndef SCANTAILOR_CORE_PAGEORIENTATIONCOLLECTOR_H_
#define SCANTAILOR_CORE_PAGEORIENTATIONCOLLECTOR_H_

#include "AbstractFilterDataCollector.h"

class OrthogonalRotation;

class PageOrientationCollector : public AbstractFilterDataCollector {
 public:
  virtual void process(const OrthogonalRotation& orientation) = 0;
};


#endif
