// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "Settings.h"

#include <DeviationProvider.h>

#include <QMutex>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <functional>
#include <iterator>
#include <list>
#include <optional>
#include <set>
#include <unordered_map>
#include <utility>

#include "AbstractRelinker.h"
#include "Guide.h"
#include "PageId.h"
#include "PageSequence.h"
#include "Params.h"
#include "RelinkablePath.h"


namespace page_layout {
class Settings::Item {
 public:
  PageId pageId;
  Margins hardMarginsMM;
  QRectF pageRect;
  QRectF contentRect;
  QSizeF contentSizeMM;
  Alignment alignment;
  bool autoMargins;
  std::size_t sequenceNumber = 0;

  Item(const PageId& pageId,
       const Margins& hardMarginsMm,
       const QRectF& pageRect,
       const QRectF& contentRect,
       const QSizeF& contentSizeMm,
       const Alignment& alignment,
       bool autoMargins);

  double hardWidthMM() const;

  double hardHeightMM() const;

  double influenceHardWidthMM() const;

  double influenceHardHeightMM() const;

  bool alignedWithOthers() const { return !alignment.isNull(); }
};


class Settings::ModifyMargins {
 public:
  ModifyMargins(const Margins& marginsMm, const bool autoMargins)
      : m_marginsMM(marginsMm), m_autoMargins(autoMargins) {}

  void operator()(Item& item) {
    item.hardMarginsMM = m_marginsMM;
    item.autoMargins = m_autoMargins;
  }

 private:
  Margins m_marginsMM;
  bool m_autoMargins;
};


class Settings::ModifyAlignment {
 public:
  explicit ModifyAlignment(const Alignment& alignment) : m_alignment(alignment) {}

  void operator()(Item& item) { item.alignment = m_alignment; }

 private:
  Alignment m_alignment;
};


class Settings::ModifyContentSize {
 public:
  ModifyContentSize(const QSizeF& contentSizeMm, const QRectF& contentRect, const QRectF& pageRect)
      : m_contentSizeMM(contentSizeMm), m_contentRect(contentRect), m_pageRect(pageRect) {}

  void operator()(Item& item) {
    item.contentSizeMM = m_contentSizeMM;
    item.contentRect = m_contentRect;
    item.pageRect = m_pageRect;
  }

 private:
  QSizeF m_contentSizeMM;
  QRectF m_contentRect;
  QRectF m_pageRect;
};


class Settings::Impl {
 public:
  Impl();

  ~Impl();

  void clear();

  void performRelinking(const AbstractRelinker& relinker);

  void removePagesMissingFrom(const PageSequence& pages);

  bool checkEverythingDefined(const PageSequence& pages, const PageId* ignore) const;

  std::unique_ptr<Params> getPageParams(const PageId& pageId) const;

  bool isParamsNull(const PageId& pageId) const;

  void setPageParams(const PageId& pageId, const Params& params);

  Params updateContentSizeAndGetParams(const PageId& pageId,
                                       const QRectF& pageRect,
                                       const QRectF& contentRect,
                                       const QSizeF& contentSizeMm,
                                       QSizeF* aggHardSizeBefore,
                                       QSizeF* aggHardSizeAfter);

  Margins getHardMarginsMM(const PageId& pageId) const;

  void setHardMarginsMM(const PageId& pageId, const Margins& marginsMm);

  Alignment getPageAlignment(const PageId& pageId) const;

  AggregateSizeChanged setPageAlignment(const PageId& pageId, const Alignment& alignment);

  AggregateSizeChanged setContentSizeMM(const PageId& pageId, const QSizeF& contentSizeMm);

  void invalidateContentSize(const PageId& pageId);

  QSizeF getAggregateHardSizeMM() const;

  QSizeF getAggregateHardSizeMMLocked() const;

  QSizeF computeAggregateHardSizeMMLocked() const;

  QSizeF getAggregateHardSizeMM(const PageId& pageId, const QSizeF& hardSizeMm, const Alignment& alignment) const;

  void setAggregateHardSizeFrozen(bool frozen);

  bool isAggregateHardSizeFrozen() const;

  bool isPageAutoMarginsEnabled(const PageId& pageId);

  void setPageAutoMarginsEnabled(const PageId& pageId, bool state);

  const DeviationProvider<PageId>& deviationProvider() const;

  std::vector<Guide>& guides();

  bool isShowingMiddleRectEnabled() const;

  void enableShowingMiddleRect(bool state);

 private:
  class Container {
   private:
    using ItemList = std::list<Item>;
    using ItemMap = std::unordered_map<PageId, ItemList::iterator, std::hash<PageId>>;

   public:
    using iterator = ItemList::const_iterator;

    struct DescWidthCompare {
      bool operator()(const Item* lhs, const Item* rhs) const {
        if (lhs->alignedWithOthers() != rhs->alignedWithOthers()) {
          return lhs->alignedWithOthers();
        }
        return compareDescending(lhs->hardWidthMM(), rhs->hardWidthMM(), lhs, rhs);
      }

     private:
      static bool compareDescending(double lhsValue, double rhsValue, const Item* lhs, const Item* rhs) {
        const bool lhsIsNan = std::isnan(lhsValue);
        const bool rhsIsNan = std::isnan(rhsValue);
        if (lhsIsNan || rhsIsNan) {
          if (lhsIsNan != rhsIsNan) {
            return !lhsIsNan;
          }
          if (lhs->sequenceNumber != rhs->sequenceNumber) {
            return lhs->sequenceNumber < rhs->sequenceNumber;
          }
          return std::less<const Item*>{}(lhs, rhs);
        }
        if (lhsValue != rhsValue) {
          return lhsValue > rhsValue;
        }
        if (lhs->sequenceNumber != rhs->sequenceNumber) {
          return lhs->sequenceNumber < rhs->sequenceNumber;
        }
        return std::less<const Item*>{}(lhs, rhs);
      }
    };

    struct DescHeightCompare {
      bool operator()(const Item* lhs, const Item* rhs) const {
        if (lhs->alignedWithOthers() != rhs->alignedWithOthers()) {
          return lhs->alignedWithOthers();
        }
        return compareDescending(lhs->hardHeightMM(), rhs->hardHeightMM(), lhs, rhs);
      }

     private:
      static bool compareDescending(double lhsValue, double rhsValue, const Item* lhs, const Item* rhs) {
        const bool lhsIsNan = std::isnan(lhsValue);
        const bool rhsIsNan = std::isnan(rhsValue);
        if (lhsIsNan || rhsIsNan) {
          if (lhsIsNan != rhsIsNan) {
            return !lhsIsNan;
          }
          if (lhs->sequenceNumber != rhs->sequenceNumber) {
            return lhs->sequenceNumber < rhs->sequenceNumber;
          }
          return std::less<const Item*>{}(lhs, rhs);
        }
        if (lhsValue != rhsValue) {
          return lhsValue > rhsValue;
        }
        if (lhs->sequenceNumber != rhs->sequenceNumber) {
          return lhs->sequenceNumber < rhs->sequenceNumber;
        }
        return std::less<const Item*>{}(lhs, rhs);
      }
    };

    using DescWidthOrder = std::set<const Item*, DescWidthCompare>;
    using DescHeightOrder = std::set<const Item*, DescHeightCompare>;

    iterator begin() const { return m_items.cbegin(); }

    iterator end() const { return m_items.cend(); }

    iterator find(const PageId& pageId) const {
      const auto it = m_itemsByPage.find(pageId);
      return it == m_itemsByPage.end() ? m_items.cend() : it->second;
    }

    bool empty() const { return m_items.empty(); }

    void clear() {
      m_descWidthOrder.clear();
      m_descHeightOrder.clear();
      m_itemsByPage.clear();
      m_items.clear();
      m_nextSequenceNumber = 0;
    }

    iterator insert(const Item& item) {
      const auto existing = m_itemsByPage.find(item.pageId);
      if (existing != m_itemsByPage.end()) {
        return existing->second;
      }

      m_items.push_back(item);
      auto itemIt = std::prev(m_items.end());
      itemIt->sequenceNumber = m_nextSequenceNumber;
      const Item* const itemPtr = &*itemIt;
      bool mapInserted = false;
      bool widthInserted = false;
      bool heightInserted = false;
      try {
        const auto result = m_itemsByPage.emplace(item.pageId, itemIt);
        if (!result.second) {
          m_items.pop_back();
          return result.first->second;
        }
        mapInserted = true;
        widthInserted = m_descWidthOrder.insert(itemPtr).second;
        heightInserted = m_descHeightOrder.insert(itemPtr).second;
        assert(widthInserted && heightInserted);
        ++m_nextSequenceNumber;
      } catch (...) {
        if (widthInserted) {
          m_descWidthOrder.erase(itemPtr);
        }
        if (heightInserted) {
          m_descHeightOrder.erase(itemPtr);
        }
        if (mapInserted) {
          m_itemsByPage.erase(item.pageId);
        }
        m_items.pop_back();
        throw;
      }
      return itemIt;
    }

    iterator insert(iterator, const Item& item) { return insert(item); }

    bool replace(iterator position, const Item& item) {
      if (position->pageId != item.pageId) {
        return false;
      }
      return modify(position, [&item](Item& current) {
        const std::size_t sequenceNumber = current.sequenceNumber;
        current = item;
        current.sequenceNumber = sequenceNumber;
      });
    }

    template <typename Modifier>
    bool modify(iterator position, Modifier modifier) {
      const auto mapIt = m_itemsByPage.find(position->pageId);
      assert(mapIt != m_itemsByPage.end());
      Item& item = *mapIt->second;
      const Item oldItem(item);
      auto widthNode = m_descWidthOrder.extract(&item);
      auto heightNode = m_descHeightOrder.extract(&item);
      try {
        modifier(item);
      } catch (...) {
        item = oldItem;
        m_descWidthOrder.insert(std::move(widthNode));
        m_descHeightOrder.insert(std::move(heightNode));
        throw;
      }
      m_descWidthOrder.insert(std::move(widthNode));
      m_descHeightOrder.insert(std::move(heightNode));
      return true;
    }

    iterator erase(iterator position) {
      const auto mapIt = m_itemsByPage.find(position->pageId);
      assert(mapIt != m_itemsByPage.end());
      const Item* const item = &*mapIt->second;
      m_descWidthOrder.erase(item);
      m_descHeightOrder.erase(item);
      m_itemsByPage.erase(mapIt);
      return m_items.erase(position);
    }

    void swap(Container& other) {
      m_items.swap(other.m_items);
      m_itemsByPage.swap(other.m_itemsByPage);
      m_descWidthOrder.swap(other.m_descWidthOrder);
      m_descHeightOrder.swap(other.m_descHeightOrder);
      std::swap(m_nextSequenceNumber, other.m_nextSequenceNumber);
    }

    const DescWidthOrder& descWidthOrder() const { return m_descWidthOrder; }

    const DescHeightOrder& descHeightOrder() const { return m_descHeightOrder; }

   private:
    ItemList m_items;
    ItemMap m_itemsByPage;
    DescWidthOrder m_descWidthOrder;
    DescHeightOrder m_descHeightOrder;
    std::size_t m_nextSequenceNumber = 0;
  };

  mutable QMutex m_mutex;
  Container m_items;
  const QRectF m_invalidRect;
  const QSizeF m_invalidSize;
  const Margins m_defaultHardMarginsMM;
  const Alignment m_defaultAlignment;
  const bool m_autoMarginsDefault;
  DeviationProvider<PageId> m_deviationProvider;
  std::vector<Guide> m_guides;
  bool m_showMiddleRect;

  std::optional<QSizeF> m_frozenAggregateHardSizeMm;
};


/*=============================== Settings ==================================*/

Settings::Settings() : m_impl(std::make_unique<Impl>()) {}

Settings::~Settings() = default;

void Settings::clear() {
  return m_impl->clear();
}

void Settings::performRelinking(const AbstractRelinker& relinker) {
  m_impl->performRelinking(relinker);
}

void Settings::removePagesMissingFrom(const PageSequence& pages) {
  m_impl->removePagesMissingFrom(pages);
}

bool Settings::checkEverythingDefined(const PageSequence& pages, const PageId* ignore) const {
  return m_impl->checkEverythingDefined(pages, ignore);
}

std::unique_ptr<Params> Settings::getPageParams(const PageId& pageId) const {
  return m_impl->getPageParams(pageId);
}

void Settings::setPageParams(const PageId& pageId, const Params& params) {
  return m_impl->setPageParams(pageId, params);
}

Params Settings::updateContentSizeAndGetParams(const PageId& pageId,
                                               const QRectF& pageRect,
                                               const QRectF& contentRect,
                                               const QSizeF& contentSizeMm,
                                               QSizeF* aggHardSizeBefore,
                                               QSizeF* aggHardSizeAfter) {
  return m_impl->updateContentSizeAndGetParams(pageId, pageRect, contentRect, contentSizeMm, aggHardSizeBefore,
                                               aggHardSizeAfter);
}

Margins Settings::getHardMarginsMM(const PageId& pageId) const {
  return m_impl->getHardMarginsMM(pageId);
}

void Settings::setHardMarginsMM(const PageId& pageId, const Margins& marginsMm) {
  m_impl->setHardMarginsMM(pageId, marginsMm);
}

Alignment Settings::getPageAlignment(const PageId& pageId) const {
  return m_impl->getPageAlignment(pageId);
}

Settings::AggregateSizeChanged Settings::setPageAlignment(const PageId& pageId, const Alignment& alignment) {
  return m_impl->setPageAlignment(pageId, alignment);
}

Settings::AggregateSizeChanged Settings::setContentSizeMM(const PageId& pageId, const QSizeF& contentSizeMm) {
  return m_impl->setContentSizeMM(pageId, contentSizeMm);
}

void Settings::invalidateContentSize(const PageId& pageId) {
  return m_impl->invalidateContentSize(pageId);
}

QSizeF Settings::getAggregateHardSizeMM() const {
  return m_impl->getAggregateHardSizeMM();
}

QSizeF Settings::getAggregateHardSizeMM(const PageId& pageId,
                                        const QSizeF& hardSizeMm,
                                        const Alignment& alignment) const {
  return m_impl->getAggregateHardSizeMM(pageId, hardSizeMm, alignment);
}

void Settings::setAggregateHardSizeFrozen(const bool frozen) {
  m_impl->setAggregateHardSizeFrozen(frozen);
}

bool Settings::isAggregateHardSizeFrozen() const {
  return m_impl->isAggregateHardSizeFrozen();
}

bool Settings::isPageAutoMarginsEnabled(const PageId& pageId) {
  return m_impl->isPageAutoMarginsEnabled(pageId);
}

void Settings::setPageAutoMarginsEnabled(const PageId& pageId, const bool state) {
  return m_impl->setPageAutoMarginsEnabled(pageId, state);
}

bool Settings::isParamsNull(const PageId& pageId) const {
  return m_impl->isParamsNull(pageId);
}

const DeviationProvider<PageId>& Settings::deviationProvider() const {
  return m_impl->deviationProvider();
}

std::vector<Guide>& Settings::guides() {
  return m_impl->guides();
}

bool Settings::isShowingMiddleRectEnabled() const {
  return m_impl->isShowingMiddleRectEnabled();
}

void Settings::enableShowingMiddleRect(const bool state) {
  m_impl->enableShowingMiddleRect(state);
}

/*============================== Settings::Item =============================*/

Settings::Item::Item(const PageId& pageId,
                     const Margins& hardMarginsMm,
                     const QRectF& pageRect,
                     const QRectF& contentRect,
                     const QSizeF& contentSizeMm,
                     const Alignment& alignment,
                     const bool autoMargins)
    : pageId(pageId),
      hardMarginsMM(hardMarginsMm),
      pageRect(pageRect),
      contentRect(contentRect),
      contentSizeMM(contentSizeMm),
      alignment(alignment),
      autoMargins(autoMargins) {}

double Settings::Item::hardWidthMM() const {
  return contentSizeMM.width() + hardMarginsMM.left() + hardMarginsMM.right();
}

double Settings::Item::hardHeightMM() const {
  return contentSizeMM.height() + hardMarginsMM.top() + hardMarginsMM.bottom();
}

double Settings::Item::influenceHardWidthMM() const {
  return alignment.isNull() ? 0.0 : hardWidthMM();
}

double Settings::Item::influenceHardHeightMM() const {
  return alignment.isNull() ? 0.0 : hardHeightMM();
}

/*============================= Settings::Impl ==============================*/

Settings::Impl::Impl()
    : m_items(),
      m_invalidRect(),
      m_invalidSize(),
      m_defaultHardMarginsMM(Margins(10.0, 5.0, 10.0, 5.0)),
      m_defaultAlignment(Alignment::TOP, Alignment::HCENTER),
      m_autoMarginsDefault(false),
      m_showMiddleRect(true) {
  m_deviationProvider.setComputeValueByKey([this](const PageId& pageId) -> double {
    auto it = m_items.find(pageId);
    if (it != m_items.end()) {
      const Margins& marginsMm = it->hardMarginsMM;
      double horHardMargins = marginsMm.left() + marginsMm.right();
      double vertHardMargins = marginsMm.top() + marginsMm.bottom();
      return std::sqrt(std::pow(horHardMargins, 2) + std::pow(vertHardMargins, 2));
    }
    return NAN;
  });
}

Settings::Impl::~Impl() = default;

void Settings::Impl::clear() {
  const QMutexLocker locker(&m_mutex);
  m_items.clear();
  m_deviationProvider.clear();
  m_frozenAggregateHardSizeMm.reset();
}

void Settings::Impl::performRelinking(const AbstractRelinker& relinker) {
  QMutexLocker locker(&m_mutex);
  Container newItems;

  for (const Item& item : m_items) {
    const RelinkablePath oldPath(item.pageId.imageId().filePath(), RelinkablePath::File);
    Item newItem(item);
    newItem.pageId.imageId().setFilePath(relinker.substitutionPathFor(oldPath));
    newItems.insert(newItem);
  }

  m_items.swap(newItems);

  m_deviationProvider.clear();
  for (const Item& item : m_items) {
    m_deviationProvider.addOrUpdate(item.pageId);
  }
}

void Settings::Impl::removePagesMissingFrom(const PageSequence& pages) {
  const QMutexLocker locker(&m_mutex);

  std::vector<PageId> sortedPages;

  sortedPages.reserve(pages.numPages());
  for (const PageInfo& page : pages) {
    sortedPages.push_back(page.id());
  }
  std::sort(sortedPages.begin(), sortedPages.end());

  Container::iterator it(m_items.begin());
  const Container::iterator end(m_items.end());
  while (it != end) {
    if (std::binary_search(sortedPages.begin(), sortedPages.end(), it->pageId)) {
      ++it;
    } else {
      m_deviationProvider.remove(it->pageId);
      it = m_items.erase(it);
    }
  }
}

bool Settings::Impl::checkEverythingDefined(const PageSequence& pages, const PageId* ignore) const {
  const QMutexLocker locker(&m_mutex);

  for (const PageInfo& pageInfo : pages) {
    if (ignore && (*ignore == pageInfo.id())) {
      continue;
    }
    const Container::iterator it(m_items.find(pageInfo.id()));
    if ((it == m_items.end()) || !it->contentSizeMM.isValid()) {
      return false;
    }
  }
  return true;
}

std::unique_ptr<Params> Settings::Impl::getPageParams(const PageId& pageId) const {
  const QMutexLocker locker(&m_mutex);

  const Container::iterator it(m_items.find(pageId));
  if (it == m_items.end()) {
    return nullptr;
  }
  return std::make_unique<Params>(it->hardMarginsMM, it->pageRect, it->contentRect, it->contentSizeMM, it->alignment,
                                  it->autoMargins);
}

void Settings::Impl::setPageParams(const PageId& pageId, const Params& params) {
  const QMutexLocker locker(&m_mutex);

  const Item newItem(pageId, params.hardMarginsMM(), params.pageRect(), params.contentRect(), params.contentSizeMM(),
                     params.alignment(), params.isAutoMarginsEnabled());

  const Container::iterator it(m_items.find(pageId));
  if (it == m_items.end()) {
    m_items.insert(it, newItem);
  } else {
    m_items.replace(it, newItem);
  }

  m_deviationProvider.addOrUpdate(pageId);
}

Params Settings::Impl::updateContentSizeAndGetParams(const PageId& pageId,
                                                     const QRectF& pageRect,
                                                     const QRectF& contentRect,
                                                     const QSizeF& contentSizeMm,
                                                     QSizeF* aggHardSizeBefore,
                                                     QSizeF* aggHardSizeAfter) {
  const QMutexLocker locker(&m_mutex);

  if (aggHardSizeBefore) {
    *aggHardSizeBefore = getAggregateHardSizeMMLocked();
  }

  const Container::iterator it(m_items.find(pageId));
  Container::iterator itemIt(it);
  if (it == m_items.end()) {
    const Item item(pageId, m_defaultHardMarginsMM, pageRect, contentRect, contentSizeMm, m_defaultAlignment,
                    m_autoMarginsDefault);
    itemIt = m_items.insert(it, item);
  } else {
    m_items.modify(it, ModifyContentSize(contentSizeMm, contentRect, pageRect));
  }

  if (aggHardSizeAfter) {
    *aggHardSizeAfter = getAggregateHardSizeMMLocked();
  }

  m_deviationProvider.addOrUpdate(pageId);
  return Params(itemIt->hardMarginsMM, itemIt->pageRect, itemIt->contentRect, itemIt->contentSizeMM, itemIt->alignment,
                itemIt->autoMargins);
}  // Settings::Impl::updateContentSizeAndGetParams

Margins Settings::Impl::getHardMarginsMM(const PageId& pageId) const {
  const QMutexLocker locker(&m_mutex);

  const Container::iterator it(m_items.find(pageId));
  if (it == m_items.end()) {
    return m_defaultHardMarginsMM;
  } else {
    return it->hardMarginsMM;
  }
}

void Settings::Impl::setHardMarginsMM(const PageId& pageId, const Margins& marginsMm) {
  const QMutexLocker locker(&m_mutex);

  const Container::iterator it(m_items.find(pageId));
  if (it == m_items.end()) {
    const Item item(pageId, marginsMm, m_invalidRect, m_invalidRect, m_invalidSize, m_defaultAlignment,
                    m_autoMarginsDefault);
    m_items.insert(it, item);
  } else {
    m_items.modify(it, ModifyMargins(marginsMm, it->autoMargins));
  }

  m_deviationProvider.addOrUpdate(pageId);
}

Alignment Settings::Impl::getPageAlignment(const PageId& pageId) const {
  const QMutexLocker locker(&m_mutex);

  const Container::iterator it(m_items.find(pageId));
  if (it == m_items.end()) {
    return m_defaultAlignment;
  } else {
    return it->alignment;
  }
}

Settings::AggregateSizeChanged Settings::Impl::setPageAlignment(const PageId& pageId, const Alignment& alignment) {
  const QMutexLocker locker(&m_mutex);

  const QSizeF aggSizeBefore(getAggregateHardSizeMMLocked());

  const Container::iterator it(m_items.find(pageId));
  if (it == m_items.end()) {
    const Item item(pageId, m_defaultHardMarginsMM, m_invalidRect, m_invalidRect, m_invalidSize, alignment,
                    m_autoMarginsDefault);
    m_items.insert(it, item);
  } else {
    m_items.modify(it, ModifyAlignment(alignment));
  }

  m_deviationProvider.addOrUpdate(pageId);

  const QSizeF aggSizeAfter(getAggregateHardSizeMMLocked());
  if (aggSizeBefore == aggSizeAfter) {
    return AGGREGATE_SIZE_UNCHANGED;
  } else {
    return AGGREGATE_SIZE_CHANGED;
  }
}

Settings::AggregateSizeChanged Settings::Impl::setContentSizeMM(const PageId& pageId, const QSizeF& contentSizeMm) {
  const QMutexLocker locker(&m_mutex);

  const QSizeF aggSizeBefore(getAggregateHardSizeMMLocked());

  const Container::iterator it(m_items.find(pageId));
  if (it == m_items.end()) {
    const Item item(pageId, m_defaultHardMarginsMM, m_invalidRect, m_invalidRect, contentSizeMm, m_defaultAlignment,
                    m_autoMarginsDefault);
    m_items.insert(it, item);
  } else {
    m_items.modify(it, ModifyContentSize(contentSizeMm, m_invalidRect, it->pageRect));
  }

  m_deviationProvider.addOrUpdate(pageId);

  const QSizeF aggSizeAfter(getAggregateHardSizeMMLocked());
  if (aggSizeBefore == aggSizeAfter) {
    return AGGREGATE_SIZE_UNCHANGED;
  } else {
    return AGGREGATE_SIZE_CHANGED;
  }
}

void Settings::Impl::invalidateContentSize(const PageId& pageId) {
  const QMutexLocker locker(&m_mutex);

  const Container::iterator it(m_items.find(pageId));
  if (it != m_items.end()) {
    m_items.modify(it, ModifyContentSize(m_invalidSize, m_invalidRect, it->pageRect));
  }

  m_deviationProvider.addOrUpdate(pageId);
}

QSizeF Settings::Impl::getAggregateHardSizeMM() const {
  const QMutexLocker locker(&m_mutex);
  return getAggregateHardSizeMMLocked();
}

QSizeF Settings::Impl::computeAggregateHardSizeMMLocked() const {
  if (m_items.empty()) {
    return QSizeF(0.0, 0.0);
  }

  const Item& maxWidthItem = **m_items.descWidthOrder().begin();
  const Item& maxHeightItem = **m_items.descHeightOrder().begin();

  const double width = maxWidthItem.influenceHardWidthMM();
  const double height = maxHeightItem.influenceHardHeightMM();
  return QSizeF(width, height);
}

QSizeF Settings::Impl::getAggregateHardSizeMMLocked() const {
  if (m_frozenAggregateHardSizeMm) {
    return *m_frozenAggregateHardSizeMm;
  }
  return computeAggregateHardSizeMMLocked();
}

void Settings::Impl::setAggregateHardSizeFrozen(const bool frozen) {
  const QMutexLocker locker(&m_mutex);
  if (frozen) {
    m_frozenAggregateHardSizeMm = computeAggregateHardSizeMMLocked();
  } else {
    m_frozenAggregateHardSizeMm.reset();
  }
}

bool Settings::Impl::isAggregateHardSizeFrozen() const {
  const QMutexLocker locker(&m_mutex);
  return m_frozenAggregateHardSizeMm.has_value();
}

QSizeF Settings::Impl::getAggregateHardSizeMM(const PageId& pageId,
                                              const QSizeF& hardSizeMm,
                                              const Alignment& alignment) const {
  const QMutexLocker locker(&m_mutex);
  if (m_frozenAggregateHardSizeMm) {
    return *m_frozenAggregateHardSizeMm;
  }

  if (alignment.isNull()) {
    return computeAggregateHardSizeMMLocked();
  }

  if (m_items.empty()) {
    return QSizeF(0.0, 0.0);
  }

  double width = 0.0;

  {
    Container::DescWidthOrder::const_iterator it(m_items.descWidthOrder().begin());
    if ((*it)->pageId != pageId) {
      width = (*it)->influenceHardWidthMM();
    } else {
      ++it;
      if (it == m_items.descWidthOrder().end()) {
        width = hardSizeMm.width();
      } else {
        width = std::max(hardSizeMm.width(), qreal((*it)->influenceHardWidthMM()));
      }
    }
  }

  double height = 0.0;

  {
    Container::DescHeightOrder::const_iterator it(m_items.descHeightOrder().begin());
    if ((*it)->pageId != pageId) {
      height = (*it)->influenceHardHeightMM();
    } else {
      ++it;
      if (it == m_items.descHeightOrder().end()) {
        height = hardSizeMm.height();
      } else {
        height = std::max(hardSizeMm.height(), qreal((*it)->influenceHardHeightMM()));
      }
    }
  }
  return QSizeF(width, height);
}  // Settings::Impl::getAggregateHardSizeMM

bool Settings::Impl::isPageAutoMarginsEnabled(const PageId& pageId) {
  const QMutexLocker locker(&m_mutex);

  const Container::iterator it(m_items.find(pageId));
  if (it == m_items.end()) {
    return m_autoMarginsDefault;
  } else {
    return it->autoMargins;
  }
}

void Settings::Impl::setPageAutoMarginsEnabled(const PageId& pageId, const bool state) {
  const QMutexLocker locker(&m_mutex);

  const Container::iterator it(m_items.find(pageId));
  if (it == m_items.end()) {
    const Item item(pageId, m_defaultHardMarginsMM, m_invalidRect, m_invalidRect, m_invalidSize, m_defaultAlignment,
                    state);
    m_items.insert(it, item);
  } else {
    m_items.modify(it, ModifyMargins(it->hardMarginsMM, state));
  }
}

bool Settings::Impl::isParamsNull(const PageId& pageId) const {
  const QMutexLocker locker(&m_mutex);
  return (m_items.find(pageId) == m_items.end());
}

const DeviationProvider<PageId>& Settings::Impl::deviationProvider() const {
  return m_deviationProvider;
}

std::vector<Guide>& Settings::Impl::guides() {
  return m_guides;
}

bool Settings::Impl::isShowingMiddleRectEnabled() const {
  return m_showMiddleRect;
}

void Settings::Impl::enableShowingMiddleRect(const bool state) {
  m_showMiddleRect = state;
}
}  // namespace page_layout