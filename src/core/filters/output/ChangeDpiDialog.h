// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#ifndef SCANTAILOR_OUTPUT_CHANGEDPIDIALOG_H_
#define SCANTAILOR_OUTPUT_CHANGEDPIDIALOG_H_

#include <QButtonGroup>
#include <QDialog>
#include <QString>
#include <memory>
#include <set>

#include "PageId.h"
#include "PageSequence.h"
#include "ui_ChangeDpiDialog.h"

class PageSelectionAccessor;
class Dpi;

namespace output {
class ChangeDpiDialog : public QDialog, private Ui::ChangeDpiDialog {
  Q_OBJECT
 public:
  ChangeDpiDialog(QWidget* parent,
                  const Dpi& dpi,
                  const PageId& curPage,
                  const PageSelectionAccessor& pageSelectionAccessor);

  ~ChangeDpiDialog() override;

 signals:

  void accepted(const std::set<PageId>& pages, const Dpi& dpi);

 private slots:

  void dpiSelectionChanged(int index);

  void dpiEditTextChanged(const QString& text);

  void onSubmit();

 private:
  PageSequence m_pages;
  std::set<PageId> m_selectedPages;
  PageId m_curPage;
  QButtonGroup* m_scopeGroup;
  int m_customItemIdx;
  QString m_customDpiString;
};
}  // namespace output
#endif  // ifndef SCANTAILOR_OUTPUT_CHANGEDPIDIALOG_H_
