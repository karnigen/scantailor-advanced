// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#ifndef SCANTAILOR_CORE_CONTENTBOXPROPAGATOR_H_
#define SCANTAILOR_CORE_CONTENTBOXPROPAGATOR_H_

#include <QSizeF>
#include <memory>
#include <set>

#include "ImageId.h"

class CompositeCacheDrivenTask;
class ProjectPages;

namespace page_layout {
class Filter;
}

/**
 * \brief Propagates content boxes from "Select Content" to "Margins" stage.
 *
 * This is necessary in the following case:\n
 * You go back from "Margins" to one of the previous stages and make
 * adjustments there to several pages.  Now you return to "Margins" and
 * expect to see the results of all your adjustments (not just the current page)
 * there.
 */
class ContentBoxPropagator {
 public:
  ContentBoxPropagator(std::shared_ptr<page_layout::Filter> pageLayoutFilter,
                       std::shared_ptr<CompositeCacheDrivenTask> task);

  ~ContentBoxPropagator();

  void propagate(const ProjectPages& pages);

  void propagateImages(const ProjectPages& pages, const std::set<ImageId>& images);

 private:
  class Collector;

  void propagate(const ProjectPages& pages, const std::set<ImageId>* images);

  std::shared_ptr<page_layout::Filter> m_pageLayoutFilter;
  std::shared_ptr<CompositeCacheDrivenTask> m_task;
};


#endif  // ifndef SCANTAILOR_CORE_CONTENTBOXPROPAGATOR_H_
