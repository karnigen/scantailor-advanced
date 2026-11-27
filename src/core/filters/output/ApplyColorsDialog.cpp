// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#include "ApplyColorsDialog.h"

#include "PageSelectionAccessor.h"

namespace output {
ApplyColorsDialog::ApplyColorsDialog(QWidget* parent,
                                     const PageId& curPage,
                                     const PageSelectionAccessor& pageSelectionAccessor)
    : QDialog(parent),
      m_pages(pageSelectionAccessor.allPages()),
      m_selectedPages(pageSelectionAccessor.selectedPages()),
      m_curPage(curPage),
      m_scopeGroup(new QButtonGroup(this)) {
  setupUi(this);
  m_scopeGroup->addButton(thisPageRB);
  m_scopeGroup->addButton(allPagesRB);
  m_scopeGroup->addButton(thisPageAndFollowersRB);
  m_scopeGroup->addButton(selectedPagesRB);
  if (m_selectedPages.size() <= 1) {
    selectedPagesRB->setEnabled(false);
    selectedPagesHint->setEnabled(false);
  }

  connect(buttonBox, SIGNAL(accepted()), this, SLOT(onSubmit()));
}

ApplyColorsDialog::~ApplyColorsDialog() = default;

void ApplyColorsDialog::onSubmit() {
  std::set<PageId> pages;

  // thisPageRB is intentionally not handled.
  if (allPagesRB->isChecked()) {
    m_pages.selectAll().swap(pages);
  } else if (thisPageAndFollowersRB->isChecked()) {
    m_pages.selectPagePlusFollowers(m_curPage).swap(pages);
  } else if (selectedPagesRB->isChecked()) {
    emit accepted(m_selectedPages);
    accept();
    return;
  }

  emit accepted(pages);

  // We assume the default connection from accepted() to accept()
  // was removed.
  accept();
}
}  // namespace output