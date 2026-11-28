// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <QButtonGroup>
#include <QDialog>
#include <memory>
#include <set>

#include "LayoutType.h"
#include "PageId.h"
#include "PageLayout.h"
#include "PageSequence.h"
#include "ui_SplitModeDialog.h"

class ProjectPages;
class PageSelectionAccessor;

namespace page_split {
class SplitModeDialog : public QDialog, private Ui::SplitModeDialog {
  Q_OBJECT
 public:
  SplitModeDialog(QWidget* parent,
                  const PageId& curPage,
                  const PageSelectionAccessor& pageSelectionAccessor,
                  LayoutType layoutType,
                  PageLayout::Type autoDetectedLayoutType,
                  bool reversePageOrder);

  ~SplitModeDialog() override;

 signals:

  void accepted(const std::set<PageId>& pages,
                LayoutType layoutType,
                bool applyCut,
                bool applyPageOrder,
                bool reversePageOrder);

 private slots:

  void autoDetectionSelected();

  void manualModeSelected();

  void onSubmit();

 private:
  LayoutType combinedLayoutType() const;

  static QIcon iconFor(LayoutType layoutType);

  PageSequence m_pages;
  std::set<PageId> m_selectedPages;
  PageId m_curPage;
  QButtonGroup* m_scopeGroup;
  LayoutType m_layoutType;
  PageLayout::Type m_autoDetectedLayoutType;
};
}  // namespace page_split
