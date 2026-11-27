// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#include "LoadFilesStatusDialog.h"

#include <QPushButton>

LoadFilesStatusDialog::LoadFilesStatusDialog(QWidget* parent) : QDialog(parent) {
  ui.setupUi(this);
  ui.tabWidget->setCurrentWidget(ui.failedTab);

  m_loadedTabNameTemplate = ui.tabWidget->tabText(0);
  m_failedTabNameTemplate = ui.tabWidget->tabText(1);

  setLoadedFiles(std::vector<QString>());
  setFailedFiles(std::vector<QString>());
}

void LoadFilesStatusDialog::setLoadedFiles(const std::vector<QString>& files) {
  ui.tabWidget->setTabText(0, m_loadedTabNameTemplate.arg(files.size()));

  QString text;
  for (const QString& file : files) {
    text.append(file);
    text.append(QChar('\n'));
  }

  ui.loadedFiles->setPlainText(text);
}

void LoadFilesStatusDialog::setFailedFiles(const std::vector<QString>& files) {
  ui.tabWidget->setTabText(1, m_failedTabNameTemplate.arg(files.size()));

  QString text;
  for (const QString& file : files) {
    text.append(file);
    text.append(QChar('\n'));
  }

  ui.failedFiles->setPlainText(text);
}

void LoadFilesStatusDialog::setOkButtonName(const QString& name) {
  ui.buttonBox->button(QDialogButtonBox::Ok)->setText(name);
}
