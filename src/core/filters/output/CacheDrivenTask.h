// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#ifndef SCANTAILOR_OUTPUT_CACHEDRIVENTASK_H_
#define SCANTAILOR_OUTPUT_CACHEDRIVENTASK_H_

#include <memory>

#include "NonCopyable.h"
#include "OutputFileNameGenerator.h"

class QPolygonF;
class PageInfo;
class AbstractFilterDataCollector;
class ImageTransformation;

namespace output {
class Settings;

class CacheDrivenTask {
  DECLARE_NON_COPYABLE(CacheDrivenTask)

 public:
  CacheDrivenTask(std::shared_ptr<Settings> settings, const OutputFileNameGenerator& outFileNameGen);

  virtual ~CacheDrivenTask();

  void process(const PageInfo& pageInfo,
               AbstractFilterDataCollector* collector,
               const ImageTransformation& xform,
               const QPolygonF& contentRectPhys);

 private:
  std::shared_ptr<Settings> m_settings;
  OutputFileNameGenerator m_outFileNameGen;
};
}  // namespace output
#endif  // ifndef SCANTAILOR_OUTPUT_CACHEDRIVENTASK_H_
