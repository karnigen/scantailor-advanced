// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include "PageId.h"
#include "PageView.h"

/**
 * The whole point of this class can be demonstrated with a few lines of code:
 * \code
 * ImageId image_id = ...;
 * SelectedPage page;
 * page.set(PageId(image_id, PageId::RIGHT_PAGE), PAGE_VIEW);
 * page.set(PageId(image_id, PageId::SINGLE_PAGE), IMAGE_VIEW);
 * page.get(PAGE_VIEW);  * \endcode
 * As seen above, this class remembers the sub-page as long as image id
 * stays the same.  Note that set(..., PAGE_VIEW) will always overwrite
 * the sub-page, while get(IMAGE_VIEW) will always return SINGLE_PAGE sub-pages.
 */
class SelectedPage {
 public:
  SelectedPage() = default;

  SelectedPage(const PageId& pageId, PageView view);

  bool isNull() const { return m_pageId.isNull(); }

  void set(const PageId& pageId, PageView view);

  PageId get(PageView view) const;

 private:
  PageId m_pageId;
};
