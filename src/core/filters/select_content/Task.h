// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <QRectF>
#include <QSizeF>
#include <memory>

#include "FilterResult.h"
#include "NonCopyable.h"
#include "PageId.h"

class TaskStatus;
class FilterData;
class DebugImages;
class ImageTransformation;
class Dpi;

namespace page_layout {
class Task;
}

namespace select_content {
class Filter;
class Settings;

class Task {
  DECLARE_NON_COPYABLE(Task)

 public:
  Task(std::shared_ptr<Filter> filter,
       std::shared_ptr<page_layout::Task> nextTask,
       std::shared_ptr<Settings> settings,
       const PageId& pageId,
       bool batch,
       bool debug);

  virtual ~Task();

  FilterResultPtr process(const TaskStatus& status, const FilterData& data);

 private:
  class UiUpdater;

  std::shared_ptr<Filter> m_filter;
  std::shared_ptr<page_layout::Task> m_nextTask;
  std::shared_ptr<Settings> m_settings;
  std::unique_ptr<DebugImages> m_dbg;
  PageId m_pageId;
  bool m_batchProcessing;
};
}  // namespace select_content
