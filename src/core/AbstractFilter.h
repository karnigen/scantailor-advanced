// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#ifndef SCANTAILOR_CORE_ABSTRACTFILTER_H_
#define SCANTAILOR_CORE_ABSTRACTFILTER_H_

#include <vector>

#include "PageOrderOption.h"
#include "PageView.h"

class FilterUiInterface;
class PageInfo;
class ProjectReader;
class ProjectWriter;
class AbstractRelinker;
class QString;
class QDomDocument;
class QDomElement;

/**
 * Filters represent processing stages, like "Deskew", "Margins" and "Output".
 */
class AbstractFilter : public std::enable_shared_from_this<AbstractFilter> {
 public:
  virtual ~AbstractFilter() = default;

  virtual QString getName() const = 0;

  virtual PageView getView() const = 0;

  virtual void selected() {}

  virtual int selectedPageOrder() const { return -1; }

  virtual void selectPageOrder(int option) {}

  virtual std::vector<PageOrderOption> pageOrderOptions() const { return std::vector<PageOrderOption>(); }

  virtual void performRelinking(const AbstractRelinker& relinker) = 0;

  virtual void preUpdateUI(FilterUiInterface* ui, const PageInfo& pageInfo) = 0;

  virtual QDomElement saveSettings(const ProjectWriter& writer, QDomDocument& doc) const = 0;

  virtual void loadSettings(const ProjectReader& reader, const QDomElement& filtersEl) = 0;

  virtual void loadDefaultSettings(const PageInfo& pageInfo) = 0;
};


#endif  // ifndef SCANTAILOR_CORE_ABSTRACTFILTER_H_
