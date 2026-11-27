// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <QDialog>

class QLabel;
class QPlainTextEdit;

class ConfigurationDialog : public QDialog {
  Q_OBJECT

 public:
  explicit ConfigurationDialog(QWidget* parent = nullptr);

 private:
  void reload();

  QLabel* m_pathLabel;
  QLabel* m_profilesPathLabel;
  QLabel* m_statusLabel;
  QPlainTextEdit* m_contents;
};
