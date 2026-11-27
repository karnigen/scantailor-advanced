// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#ifndef SCANTAILOR_PAGE_LAYOUT_CACHEDRIVENTASK_H_
#define SCANTAILOR_PAGE_LAYOUT_CACHEDRIVENTASK_H_

#include <QPolygonF>
#include <memory>

#include "NonCopyable.h"

class QRectF;
class PageInfo;
class AbstractFilterDataCollector;
class ImageTransformation;

namespace output {
class CacheDrivenTask;
}

namespace page_layout {
class Settings;

class CacheDrivenTask {
  DECLARE_NON_COPYABLE(CacheDrivenTask)

 public:
  CacheDrivenTask(std::shared_ptr<output::CacheDrivenTask> nextTask, std::shared_ptr<Settings> settings);

  virtual ~CacheDrivenTask();

  void process(const PageInfo& pageInfo,
               AbstractFilterDataCollector* collector,
               const ImageTransformation& xform,
               const QRectF& pageRect,
               const QRectF& contentRect,
               bool pageBoundaryEnabled = false);

 private:
  std::shared_ptr<output::CacheDrivenTask> m_nextTask;
  std::shared_ptr<Settings> m_settings;
};
}  // namespace page_layout
#endif  // ifndef SCANTAILOR_PAGE_LAYOUT_CACHEDRIVENTASK_H_
