// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <QDialog>
#include <memory>

#include "PropertySet.h"
#include "ui_PictureZonePropDialog.h"

namespace output {
class PictureZonePropDialog : public QDialog {
  Q_OBJECT
 public:
  explicit PictureZonePropDialog(std::shared_ptr<PropertySet> props, QWidget* parent = nullptr);

 signals:
  void updated();

 private slots:
  void itemToggled(bool selected);

 private:
  Ui::PictureZonePropDialog ui;
  std::shared_ptr<PropertySet> m_props;
};
}  // namespace output
