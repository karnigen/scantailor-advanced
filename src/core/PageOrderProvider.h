// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#ifndef SCANTAILOR_CORE_PAGEORDERPROVIDER_H_
#define SCANTAILOR_CORE_PAGEORDERPROVIDER_H_

#include <memory>

class PageId;

/**
 * A base interface for different page ordering strategies.
 */
class PageOrderProvider : public std::enable_shared_from_this<PageOrderProvider> {
 public:
  virtual ~PageOrderProvider() = default;

  /**
   * Returns true if \p lhsPage precedes \p rhsPage.
   * \p lhsIncomplete and \p rhsIncomplete indicate whether
   * a page is represented by IncompleteThumbnail.
   */
  virtual bool precedes(const PageId& lhsPage, bool lhsIncomplete, const PageId& rhsPage, bool rhsIncomplete) const = 0;

  /**
   * Returns a provider view for a sequence of comparisons.
   *
   * Implementations reading mutable state must freeze the ordering values here, so that a
   * comparator used in a single sort never observes two different snapshots of the data.
   * Providers whose ordering depends only on the comparison arguments or on immutable data
   * return themselves.
   */
  virtual std::shared_ptr<const PageOrderProvider> snapshot() const = 0;

  virtual std::shared_ptr<const PageOrderProvider> reversed() const;
};


#endif
