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
#include "ui_ApplyDialog.h"

class PageSelectionAccessor;

namespace deskew {
class ApplyDialog : public QDialog, private Ui::ApplyDialog {
  Q_OBJECT
 public:
  ApplyDialog(QWidget* parent, const PageId& curPage, const PageSelectionAccessor& pageSelectionAccessor);

  ~ApplyDialog() override;

 signals:

  void appliedTo(const std::set<PageId>& pages, bool applyDeskew, bool applyOblique);

  void appliedToAllPages(const std::set<PageId>& pages, bool applyDeskew, bool applyOblique);

 private slots:

  void onSubmit();

 private:
  PageSequence m_pages;
  PageId m_curPage;
  std::set<PageId> m_selectedPages;
  QButtonGroup* m_scopeGroup;
};
}  // namespace deskew
