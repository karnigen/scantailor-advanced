// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#ifndef SCANTAILOR_APP_LOADFILESSTATUSDIALOG_H_
#define SCANTAILOR_APP_LOADFILESSTATUSDIALOG_H_

#include <QString>
#include <vector>

#include "ui_LoadFilesStatusDialog.h"

class LoadFilesStatusDialog : public QDialog {
 public:
  explicit LoadFilesStatusDialog(QWidget* parent = nullptr);

  void setLoadedFiles(const std::vector<QString>& files);

  void setFailedFiles(const std::vector<QString>& failed);

  void setOkButtonName(const QString& name);

 private:
  Ui::LoadFilesStatusDialog ui;
  QString m_loadedTabNameTemplate;
  QString m_failedTabNameTemplate;
};


#endif
