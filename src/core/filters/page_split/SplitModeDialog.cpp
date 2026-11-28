// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#include "SplitModeDialog.h"

#include <core/IconProvider.h>

#include <cassert>
#include <iostream>

#include "PageSelectionAccessor.h"

namespace page_split {
SplitModeDialog::SplitModeDialog(QWidget* const parent,
                                 const PageId& curPage,
                                 const PageSelectionAccessor& pageSelectionAccessor,
                                 const LayoutType layoutType,
                                 const PageLayout::Type autoDetectedLayoutType,
                                 const bool reversePageOrder)
    : QDialog(parent),
      m_pages(pageSelectionAccessor.allPages()),
      m_selectedPages(pageSelectionAccessor.selectedPages()),
      m_curPage(curPage),
      m_scopeGroup(new QButtonGroup(this)),
      m_layoutType(layoutType),
      m_autoDetectedLayoutType(autoDetectedLayoutType) {
  setupUi(this);
  m_scopeGroup->addButton(thisPageRB);
  m_scopeGroup->addButton(allPagesRB);
  m_scopeGroup->addButton(thisPageAndFollowersRB);
  m_scopeGroup->addButton(thisEveryOtherRB);
  m_scopeGroup->addButton(everyOtherRB);
  m_scopeGroup->addButton(selectedPagesRB);
  m_scopeGroup->addButton(everyOtherSelectedRB);
  if (m_selectedPages.size() <= 1) {
    selectedPagesRB->setEnabled(false);
    selectedPagesHint->setEnabled(false);
    everyOtherSelectedRB->setEnabled(false);
    everyOtherSelectedHint->setEnabled(false);
  }

  layoutTypeLabel->setPixmap(iconFor(m_layoutType).pixmap(32, 32));
  if (m_layoutType == AUTO_LAYOUT_TYPE) {
    modeAuto->setChecked(true);
    applyCutOption->setEnabled(false);
  } else {
    modeManual->setChecked(true);
    applyCutOption->setEnabled(true);
  }

  const bool uncutPage = m_layoutType == SINGLE_PAGE_UNCUT;
  groupBox->setVisible(!uncutPage);
  optionsBox->setVisible(!uncutPage);
  reversePageOrderOption->setVisible(combinedLayoutType() == TWO_PAGES);
  reversePageOrderOption->setChecked(reversePageOrder);

  connect(modeAuto, SIGNAL(pressed()), this, SLOT(autoDetectionSelected()));
  connect(modeManual, SIGNAL(pressed()), this, SLOT(manualModeSelected()));
  connect(buttonBox, SIGNAL(accepted()), this, SLOT(onSubmit()));
}

SplitModeDialog::~SplitModeDialog() = default;

void SplitModeDialog::autoDetectionSelected() {
  layoutTypeLabel->setPixmap(iconFor(AUTO_LAYOUT_TYPE).pixmap(32, 32));
  applyCutOption->setEnabled(false);
  applyCutOption->setChecked(false);
}

void SplitModeDialog::manualModeSelected() {
  layoutTypeLabel->setPixmap(iconFor(combinedLayoutType()).pixmap(32, 32));
  applyCutOption->setEnabled(true);
}

void SplitModeDialog::onSubmit() {
  LayoutType layoutType = AUTO_LAYOUT_TYPE;
  if (m_layoutType == SINGLE_PAGE_UNCUT) {
    layoutType = SINGLE_PAGE_UNCUT;
  } else if (modeManual->isChecked()) {
    layoutType = combinedLayoutType();
  }

  const bool applyCut = applyCutOption->isVisible() && applyCutOption->isChecked();
  const bool applyPageOrder = reversePageOrderOption->isVisible();
  const bool reversePageOrder = reversePageOrderOption->isChecked();
  std::set<PageId> pages;

  if (thisPageRB->isChecked()) {
    pages.insert(m_curPage);
  } else if (allPagesRB->isChecked()) {
    m_pages.selectAll().swap(pages);
  } else if (thisPageAndFollowersRB->isChecked()) {
    m_pages.selectPagePlusFollowers(m_curPage).swap(pages);
  } else if (selectedPagesRB->isChecked()) {
    emit accepted(m_selectedPages, layoutType, applyCut, applyPageOrder, reversePageOrder);
    accept();
    return;
  } else if (everyOtherRB->isChecked()) {
    m_pages.selectEveryOther(m_curPage).swap(pages);
  } else if (thisEveryOtherRB->isChecked()) {
    m_pages.selectThisPageAndFollowingEveryOther(m_curPage).swap(pages);
  } else if (everyOtherSelectedRB->isChecked()) {
    m_pages.selectEveryOtherInSubsetFromPage(m_curPage, m_selectedPages).swap(pages);
  }

  emit accepted(pages, layoutType, applyCut, applyPageOrder, reversePageOrder);
  // We assume the default connection from accepted() to accept()
  // was removed.
  accept();
}  // SplitModeDialog::onSubmit

LayoutType SplitModeDialog::combinedLayoutType() const {
  if (m_layoutType != AUTO_LAYOUT_TYPE) {
    return m_layoutType;
  }

  switch (m_autoDetectedLayoutType) {
    case PageLayout::SINGLE_PAGE_UNCUT:
      return SINGLE_PAGE_UNCUT;
    case PageLayout::SINGLE_PAGE_CUT:
      return PAGE_PLUS_OFFCUT;
    case PageLayout::TWO_PAGES:
      return TWO_PAGES;
  }

  assert(!"Unreachable");
  return AUTO_LAYOUT_TYPE;
}

QIcon SplitModeDialog::iconFor(const LayoutType layoutType) {
  QIcon icon;
  switch (layoutType) {
    case AUTO_LAYOUT_TYPE:
      icon = IconProvider::getInstance().getIcon("layout_type_auto");
      break;
    case SINGLE_PAGE_UNCUT:
      icon = IconProvider::getInstance().getIcon("single_page_uncut");
      break;
    case PAGE_PLUS_OFFCUT:
      icon = IconProvider::getInstance().getIcon("right_page_plus_offcut");
      break;
    case TWO_PAGES:
      icon = IconProvider::getInstance().getIcon("two_pages");
      break;
  }
  return icon;
}
}  // namespace page_split