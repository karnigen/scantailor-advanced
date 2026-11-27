// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#ifndef SCANTAILOR_CORE_LOADFILETASK_H_
#define SCANTAILOR_CORE_LOADFILETASK_H_

#include <memory>

#include "BackgroundTask.h"
#include "FilterResult.h"
#include "ImageId.h"
#include "ImageMetadata.h"
#include "NonCopyable.h"
#include "PageId.h"

class ThumbnailPixmapCache;
class PageInfo;
class ProjectPages;
class QImage;

namespace fix_orientation {
class Task;
}

class LoadFileTask : public BackgroundTask {
  DECLARE_NON_COPYABLE(LoadFileTask)

 public:
  LoadFileTask(Type type,
               const PageInfo& page,
               std::shared_ptr<ThumbnailPixmapCache> thumbnailCache,
               std::shared_ptr<ProjectPages> pages,
               std::shared_ptr<fix_orientation::Task> nextTask);

  ~LoadFileTask() override;

  FilterResultPtr operator()() override;

  FilterResultPtr exceptionResult(const QString& message) override;

 private:
  class ErrorResult;

  void updateImageSizeIfChanged(const QImage& image);

  void overrideDpi(QImage& image) const;

  void convertToSupportedFormat(QImage& image) const;

  std::shared_ptr<ThumbnailPixmapCache> m_thumbnailCache;
  ImageId m_imageId;
  PageId m_pageId;
  ImageMetadata m_imageMetadata;
  const std::shared_ptr<ProjectPages> m_pages;
  const std::shared_ptr<fix_orientation::Task> m_nextTask;
};


#endif  // ifndef SCANTAILOR_CORE_LOADFILETASK_H_
