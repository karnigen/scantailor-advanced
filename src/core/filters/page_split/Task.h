// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <memory>

#include "FilterResult.h"
#include "NonCopyable.h"
#include "PageInfo.h"

class TaskStatus;
class FilterData;
class DebugImages;
class ProjectPages;
class QImage;

namespace deskew {
class Task;
}

namespace page_split {
class Filter;
class Settings;

class PageLayout;

class Task {
  DECLARE_NON_COPYABLE(Task)

 public:
  Task(std::shared_ptr<Filter> filter,
       std::shared_ptr<Settings> settings,
       std::shared_ptr<ProjectPages> pages,
       std::shared_ptr<deskew::Task> nextTask,
       const PageInfo& pageInfo,
       bool batchProcessing,
       bool debug);

  virtual ~Task();

  FilterResultPtr process(const TaskStatus& status, const FilterData& data);

 private:
  class UiUpdater;

  std::shared_ptr<Filter> m_filter;
  std::shared_ptr<Settings> m_settings;
  std::shared_ptr<ProjectPages> m_pages;
  std::shared_ptr<deskew::Task> m_nextTask;
  std::unique_ptr<DebugImages> m_dbg;
  PageInfo m_pageInfo;
  bool m_batchProcessing;
};
}  // namespace page_split
