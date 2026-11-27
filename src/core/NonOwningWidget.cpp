// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#include "NonOwningWidget.h"

NonOwningWidget::NonOwningWidget(QWidget* parent) : QWidget(parent) {}

NonOwningWidget::~NonOwningWidget() {
  for (QObject* child : children()) {
    if (auto* widget = dynamic_cast<QWidget*>(child)) {
      widget->setParent(nullptr);
    }
  }
}
