// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#include "SelectedPage.h"

SelectedPage::SelectedPage(const PageId& pageId, PageView view) {
  set(pageId, view);
}

void SelectedPage::set(const PageId& pageId, PageView view) {
  if ((view == PAGE_VIEW) || (pageId.imageId() != m_pageId.imageId())) {
    m_pageId = pageId;
  }
}

PageId SelectedPage::get(PageView view) const {
  if (view == PAGE_VIEW) {
    return m_pageId;
  } else {
    return PageId(m_pageId.imageId(), PageId::SINGLE_PAGE);
  }
}
