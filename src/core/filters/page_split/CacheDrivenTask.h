// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <memory>

#include "NonCopyable.h"

class QSizeF;
class PageInfo;
class AbstractFilterDataCollector;
class ImageTransformation;
class ProjectPages;

namespace deskew {
class CacheDrivenTask;
}

namespace page_split {
class Settings;

class CacheDrivenTask {
  DECLARE_NON_COPYABLE(CacheDrivenTask)

 public:
  CacheDrivenTask(std::shared_ptr<Settings> settings,
                  std::shared_ptr<ProjectPages> projectPages,
                  std::shared_ptr<deskew::CacheDrivenTask> nextTask);

  virtual ~CacheDrivenTask();

  void process(const PageInfo& pageInfo, AbstractFilterDataCollector* collector, const ImageTransformation& xform);

 private:
  std::shared_ptr<deskew::CacheDrivenTask> m_nextTask;
  std::shared_ptr<Settings> m_settings;
  std::shared_ptr<ProjectPages> m_projectPages;
};
}  // namespace page_split
