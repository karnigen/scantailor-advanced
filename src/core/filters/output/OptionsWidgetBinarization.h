// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <PageId.h>

#include <QWidget>

namespace output {
class OptionsWidgetBinarization : public QWidget {
  Q_OBJECT
 public:
  virtual void updateUi(const PageId& m_pageId) = 0;

 signals:

  /**
   * \brief To be emitted by subclasses when their state has changed.
   */
  void stateChanged();
};
}  // namespace output
