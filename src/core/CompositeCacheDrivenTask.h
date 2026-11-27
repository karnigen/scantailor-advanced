// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#ifndef SCANTAILOR_CORE_COMPOSITECACHEDRIVENTASK_H_
#define SCANTAILOR_CORE_COMPOSITECACHEDRIVENTASK_H_


class PageInfo;
class AbstractFilterDataCollector;

class CompositeCacheDrivenTask {
 public:
  virtual ~CompositeCacheDrivenTask() = default;

  virtual void process(const PageInfo& pageInfo, AbstractFilterDataCollector* collector) = 0;
};


#endif
