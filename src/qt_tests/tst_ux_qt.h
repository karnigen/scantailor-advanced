// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#ifndef TST_UX_QT_H
#define TST_UX_QT_H

#include <QObject>

class TestUxQt : public QObject {
  Q_OBJECT
 private slots:
  void testButtonClickEmitsSignal();
  void testCheckBoxToggleChangesState();
  void testLineEditAcceptsInput();
  void testCollapsibleGroupBoxStateAndSignal();
  void testErrorWidgetCreation();
  void testConfigurationDialogShowsPathsAndReloadsSettings();
  void testSingleColumnSettingUpdatesButtonState();
};

#endif
