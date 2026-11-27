// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#include "ContentBoxPropagator.h"

#include <utility>

#include "CompositeCacheDrivenTask.h"
#include "ContentBoxCollector.h"
#include "ImageTransformation.h"
#include "PageSequence.h"
#include "ProjectPages.h"
#include "filters/page_layout/Filter.h"

class ContentBoxPropagator::Collector : public ContentBoxCollector {
 public:
  Collector();

  void process(const ImageTransformation& xform,
               const QRectF& pageRect,
               const QRectF& contentRect,
               bool pageBoundaryEnabled) override;

  bool collected() const { return m_collected; }

  const ImageTransformation& xform() const { return m_xform; }

  const QRectF& pageRect() const { return m_pageRect; }

  const QRectF& contentRect() const { return m_contentRect; }

  bool pageBoundaryEnabled() const { return m_pageBoundaryEnabled; }

 private:
  ImageTransformation m_xform;
  QRectF m_pageRect;
  QRectF m_contentRect;
  bool m_pageBoundaryEnabled = false;
  bool m_collected;
};


ContentBoxPropagator::ContentBoxPropagator(std::shared_ptr<page_layout::Filter> pageLayoutFilter,
                                           std::shared_ptr<CompositeCacheDrivenTask> task)
    : m_pageLayoutFilter(std::move(pageLayoutFilter)), m_task(std::move(task)) {}

ContentBoxPropagator::~ContentBoxPropagator() = default;

void ContentBoxPropagator::propagate(const ProjectPages& pages) {
  propagate(pages, nullptr);
}

void ContentBoxPropagator::propagateImages(const ProjectPages& pages, const std::set<ImageId>& images) {
  propagate(pages, &images);
}

void ContentBoxPropagator::propagate(const ProjectPages& pages, const std::set<ImageId>* images) {
  const PageSequence sequence(pages.toPageSequence(PAGE_VIEW));

  for (const PageInfo& pageInfo : sequence) {
    if (images && images->find(pageInfo.imageId()) == images->end()) {
      continue;
    }
    Collector collector;
    m_task->process(pageInfo, &collector);
    if (collector.collected()) {
      m_pageLayoutFilter->setContentBox(pageInfo.id(), collector.xform(), collector.pageRect(), collector.contentRect(),
                                        collector.pageBoundaryEnabled());
    } else {
      m_pageLayoutFilter->invalidateContentBox(pageInfo.id());
    }
  }
}

/*=================== ContentBoxPropagator::Collector ====================*/

ContentBoxPropagator::Collector::Collector() : m_xform(QRectF(0, 0, 1, 1), Dpi(300, 300)), m_collected(false) {}

void ContentBoxPropagator::Collector::process(const ImageTransformation& xform,
                                              const QRectF& pageRect,
                                              const QRectF& contentRect,
                                              const bool pageBoundaryEnabled) {
  m_xform = xform;
  m_pageRect = pageRect;
  m_contentRect = contentRect;
  m_pageBoundaryEnabled = pageBoundaryEnabled;
  m_collected = true;
}
