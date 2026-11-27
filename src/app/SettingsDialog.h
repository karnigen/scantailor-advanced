// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#ifndef SCANTAILOR_APP_SETTINGSDIALOG_H_
#define SCANTAILOR_APP_SETTINGSDIALOG_H_

#include <QDialog>

#include "ui_SettingsDialog.h"

class SettingsDialog : public QDialog {
  Q_OBJECT
 public:
  explicit SettingsDialog(QWidget* parent = nullptr);

  ~SettingsDialog() override;

 public slots:
  void accept() override;

 signals:
  void settingsChanged();

 private:
  bool commitChanges();

 private slots:
  void blackOnWhiteDetectionToggled(bool checked);

 private:
  Ui::SettingsDialog ui;
};


#endif
