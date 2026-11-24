// Copyright (C) 2026  ScanTailor Advanced contributors
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_CORE_PAGESIZEORDERSNAPSHOT_H_
#define SCANTAILOR_CORE_PAGESIZEORDERSNAPSHOT_H_

#include <QSizeF>

#include <unordered_map>
#include <utility>

#include "PageId.h"
#include "PageOrderProvider.h"

class PageSizeOrderSnapshot : public PageOrderProvider {
 public:
  enum class Axis { WIDTH, HEIGHT };

  PageSizeOrderSnapshot(std::unordered_map<PageId, QSizeF> sizes, const Axis axis)
      : m_sizes(std::move(sizes)), m_axis(axis) {}

  bool precedes(const PageId& lhsPage,
                const bool lhsIncomplete,
                const PageId& rhsPage,
                const bool rhsIncomplete) const override {
    const QSizeF lhsSize(sizeFor(lhsPage));
    const QSizeF rhsSize(sizeFor(rhsPage));
    const bool lhsValid = !lhsIncomplete && lhsSize.isValid();
    const bool rhsValid = !rhsIncomplete && rhsSize.isValid();

    if (lhsValid != rhsValid) {
      return lhsValid;
    }
    return (m_axis == Axis::WIDTH) ? (lhsSize.width() < rhsSize.width()) : (lhsSize.height() < rhsSize.height());
  }

 private:
  QSizeF sizeFor(const PageId& pageId) const {
    const auto it = m_sizes.find(pageId);
    return it == m_sizes.end() ? QSizeF() : it->second;
  }

  const std::unordered_map<PageId, QSizeF> m_sizes;
  const Axis m_axis;
};

#endif  // SCANTAILOR_CORE_PAGESIZEORDERSNAPSHOT_H_
