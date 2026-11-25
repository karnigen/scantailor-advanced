// Copyright (C) 2026  ScanTailor Advanced contributors
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_CORE_PAGESIZEORDERSNAPSHOT_H_
#define SCANTAILOR_CORE_PAGESIZEORDERSNAPSHOT_H_

#include <QSizeF>

#include <memory>
#include <unordered_map>
#include <utility>

#include "Margins.h"
#include "PageId.h"
#include "PageOrderProvider.h"

/**
 * Immutable ordering of pages by one of their dimensions.
 *
 * The comparison rule is shared with the live order providers, so a snapshot and the provider it
 * was taken from can never order pages differently.
 */
class PageSizeOrderSnapshot : public PageOrderProvider {
 public:
  enum class Axis { WIDTH, HEIGHT };

  PageSizeOrderSnapshot(std::unordered_map<PageId, QSizeF> sizes, const Axis axis)
      : m_sizes(std::move(sizes)), m_axis(axis) {}

  /**
   * The size used for ordering: the content size plus the hard margins.
   */
  static QSizeF sizeMM(const QSizeF& contentSizeMM, const Margins& hardMarginsMM) {
    QSizeF size(contentSizeMM);
    size += QSizeF(hardMarginsMM.left() + hardMarginsMM.right(), hardMarginsMM.top() + hardMarginsMM.bottom());
    return size;
  }

  /**
   * The single definition of "order by size" shared by the live providers and their snapshots.
   * Pages without a known size go to the back.
   */
  static bool comparesBefore(const QSizeF& lhsSize,
                             bool lhsIncomplete,
                             const QSizeF& rhsSize,
                             bool rhsIncomplete,
                             const Axis axis) {
    const bool lhsValid = !lhsIncomplete && lhsSize.isValid();
    const bool rhsValid = !rhsIncomplete && rhsSize.isValid();

    if (lhsValid != rhsValid) {
      return lhsValid;
    }
    return (axis == Axis::WIDTH) ? (lhsSize.width() < rhsSize.width()) : (lhsSize.height() < rhsSize.height());
  }

  bool precedes(const PageId& lhsPage,
                const bool lhsIncomplete,
                const PageId& rhsPage,
                const bool rhsIncomplete) const override {
    return comparesBefore(sizeFor(lhsPage), lhsIncomplete, sizeFor(rhsPage), rhsIncomplete, m_axis);
  }

  std::shared_ptr<const PageOrderProvider> snapshot() const override { return shared_from_this(); }

 private:
  QSizeF sizeFor(const PageId& pageId) const {
    const auto it = m_sizes.find(pageId);
    return it == m_sizes.end() ? QSizeF() : it->second;
  }

  const std::unordered_map<PageId, QSizeF> m_sizes;
  const Axis m_axis;
};

#endif  // SCANTAILOR_CORE_PAGESIZEORDERSNAPSHOT_H_
