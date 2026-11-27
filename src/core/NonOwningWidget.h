// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#ifndef SCANTAILOR_CORE_NONOWNINGWIDGET_H_
#define SCANTAILOR_CORE_NONOWNINGWIDGET_H_

#include <QWidget>

/**
 * \brief Your normal QWidget, except it doesn't delete its children with itself,
 *        rather it calls setParent(0) on them.
 */
class NonOwningWidget : public QWidget {
 public:
  explicit NonOwningWidget(QWidget* parent = nullptr);

  ~NonOwningWidget() override;
};


#endif
