// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#include "PageOrientationPropagator.h"

#include <utility>

#include "CompositeCacheDrivenTask.h"
#include "OrthogonalRotation.h"
#include "PageOrientationCollector.h"
#include "PageSequence.h"
#include "ProjectPages.h"
#include "filters/page_split/Filter.h"

class PageOrientationPropagator::Collector : public PageOrientationCollector {
 public:
  void process(const OrthogonalRotation& orientation) override { m_orientation = orientation; }

  const OrthogonalRotation& orientation() const { return m_orientation; }

 private:
  OrthogonalRotation m_orientation;
};


PageOrientationPropagator::PageOrientationPropagator(std::shared_ptr<page_split::Filter> pageSplitFilter,
                                                     std::shared_ptr<CompositeCacheDrivenTask> task)
    : m_pageSplitFilter(std::move(pageSplitFilter)), m_task(std::move(task)) {}

PageOrientationPropagator::~PageOrientationPropagator() = default;

void PageOrientationPropagator::propagate(const ProjectPages& pages) {
  const PageSequence sequence(pages.toPageSequence(PAGE_VIEW));

  for (const PageInfo& pageInfo : sequence) {
    Collector collector;
    m_task->process(pageInfo, &collector);
    m_pageSplitFilter->pageOrientationUpdate(pageInfo.imageId(), collector.orientation());
  }
}
