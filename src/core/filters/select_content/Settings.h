// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#ifndef SCANTAILOR_SELECT_CONTENT_SETTINGS_H_
#define SCANTAILOR_SELECT_CONTENT_SETTINGS_H_

#include <DeviationProvider.h>

#include <QMutex>
#include <memory>
#include <unordered_map>

#include "NonCopyable.h"
#include "PageId.h"
#include "Params.h"

class AbstractRelinker;

namespace select_content {
class Settings {
  DECLARE_NON_COPYABLE(Settings)

 public:
  Settings();

  virtual ~Settings();

  void clear();

  void performRelinking(const AbstractRelinker& relinker);

  void setPageParams(const PageId& pageId, const Params& params);

  void clearPageParams(const PageId& pageId);

  std::unique_ptr<Params> getPageParams(const PageId& pageId) const;

  /**
   * The content size used for ordering pages by width or height.
   */
  QSizeF getContentSizeMM(const PageId& pageId) const;

  std::unordered_map<PageId, QSizeF> getContentSizesSnapshot() const;

  bool isParamsNull(const PageId& pageId) const;

  QSizeF pageDetectionBox() const;

  void setPageDetectionBox(QSizeF size);

  double pageDetectionTolerance() const;

  void setPageDetectionTolerance(double tolerance);

  const DeviationProvider<PageId>& deviationProvider() const;

 private:
  using PageParams = std::unordered_map<PageId, Params>;

  mutable QMutex m_mutex;
  PageParams m_pageParams;
  QSizeF m_pageDetectionBox;
  double m_pageDetectionTolerance;
  DeviationProvider<PageId> m_deviationProvider;
};
}  // namespace select_content
#endif  // ifndef SCANTAILOR_SELECT_CONTENT_SETTINGS_H_
