// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <QButtonGroup>
#include <QDialog>
#include <memory>
#include <set>

#include "PageId.h"
#include "PageSequence.h"
#include "ui_ApplyColorsDialog.h"

class PageSelectionAccessor;

namespace output {
class ApplyColorsDialog : public QDialog, private Ui::ApplyColorsDialog {
  Q_OBJECT
 public:
  ApplyColorsDialog(QWidget* parent, const PageId& pageId, const PageSelectionAccessor& pageSelectionAccessor);

  ~ApplyColorsDialog() override;

 signals:

  void accepted(const std::set<PageId>& pages);

 private slots:

  void onSubmit();

 private:
  PageSequence m_pages;
  std::set<PageId> m_selectedPages;
  PageId m_curPage;
  QButtonGroup* m_scopeGroup;
};
}  // namespace output
