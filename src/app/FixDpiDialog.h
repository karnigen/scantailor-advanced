// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#ifndef SCANTAILOR_APP_FIXDPIDIALOG_H_
#define SCANTAILOR_APP_FIXDPIDIALOG_H_

#include <QDialog>
#include <QItemSelection>
#include <QPalette>
#include <QSize>
#include <QString>
#include <memory>
#include <vector>

#include "Dpi.h"
#include "ImageFileInfo.h"
#include "ImageMetadata.h"
#include "ui_FixDpiDialog.h"

class QItemSelection;
class QTableView;

class FixDpiDialog : public QDialog, private Ui::FixDpiDialog {
  Q_OBJECT
 public:
  explicit FixDpiDialog(const std::vector<ImageFileInfo>& files, QWidget* parent = nullptr);

  ~FixDpiDialog() override;

  const std::vector<ImageFileInfo>& files() const;

 private slots:

  void selectionChanged(const QItemSelection& selection);

  void dpiComboChangedByUser(int index);

  void dpiValueChanged();

  void applyClicked();

  void applyAllClicked();

 private:
  class PageTableModel;
  class PageTableSortProxyModel;

  void enableDisableOkButton();

  std::vector<int> selectedRows() const;

  void updateDpiFromSelection(const QItemSelection& selection);

  void updateDpiStatistics();

  void resetDpiForm();

  void setDpiForm(const ImageMetadata& metadata);

  void updateDpiCombo();

  void decorateDpiInputField(QLineEdit* field, ImageMetadata::DpiStatus dpiStatus) const;

  std::unique_ptr<PageTableModel> m_pages;
  std::unique_ptr<PageTableSortProxyModel> m_sortProxy;
  int m_sortColumn = -1;
  Qt::SortOrder m_sortOrder = Qt::AscendingOrder;
  std::vector<QSize> m_selectedItemPixelSizes;
  QPalette m_normalPalette;
  QPalette m_errorPalette;
};


#endif  // ifndef SCANTAILOR_APP_FIXDPIDIALOG_H_
