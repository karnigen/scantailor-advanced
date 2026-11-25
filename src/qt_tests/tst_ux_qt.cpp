// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "tst_ux_qt.h"

#include <CollapsibleGroupBox.h>
#include <ConfigurationDialog.h>
#include <ErrorWidget.h>
#include <ThumbnailViewMode.h>
#include <core/SettingsStore.h>

#include <QCheckBox>
#include <QClipboard>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSignalSpy>
#include <QToolButton>
#include <QWidget>
#include <QtTest/QtTest>

void TestUxQt::testButtonClickEmitsSignal() {
  QPushButton button("Click me");
  QSignalSpy spy(&button, &QPushButton::clicked);
  QVERIFY(spy.isValid());

  QTest::mouseClick(&button, Qt::LeftButton);
  QCOMPARE(spy.count(), 1);

  QTest::mouseClick(&button, Qt::LeftButton);
  QCOMPARE(spy.count(), 2);
}

void TestUxQt::testCheckBoxToggleChangesState() {
  QCheckBox box("Option");
  QVERIFY(!box.isChecked());

  box.setChecked(true);
  QVERIFY(box.isChecked());

  box.setChecked(false);
  QVERIFY(!box.isChecked());

  QSignalSpy spy(&box, &QCheckBox::toggled);
  box.setChecked(true);
  QCOMPARE(spy.count(), 1);
  QVERIFY(box.isChecked());
}

void TestUxQt::testLineEditAcceptsInput() {
  QLineEdit edit;
  QCOMPARE(edit.text(), QString());

  QTest::keyClicks(&edit, "Hello UX");
  QCOMPARE(edit.text(), QString("Hello UX"));

  edit.clear();
  QTest::keyClicks(&edit, "123");
  QCOMPARE(edit.text(), QString("123"));
}

void TestUxQt::testCollapsibleGroupBoxStateAndSignal() {
  CollapsibleGroupBox group("Options");
  QSignalSpy spy(&group, &CollapsibleGroupBox::collapsedStateChanged);
  QVERIFY(spy.isValid());

  QVERIFY(!group.isCollapsed());

  group.setCollapsed(true);
  QVERIFY(group.isCollapsed());
  QCOMPARE(spy.count(), 1);
  QCOMPARE(spy.at(0).at(0).toBool(), true);

  group.setCollapsed(false);
  QVERIFY(!group.isCollapsed());
  QCOMPARE(spy.count(), 2);
  QCOMPARE(spy.at(1).at(0).toBool(), false);

  group.setCollapsed(true);
  auto* collapseButton = group.findChild<QToolButton*>(QStringLiteral("collapseButton"));
  QVERIFY(collapseButton);
  group.show();
  QTRY_VERIFY(collapseButton->isVisible());
  QTest::mouseClick(collapseButton, Qt::LeftButton);
  QVERIFY(!group.isCollapsed());
}

void TestUxQt::testErrorWidgetCreation() {
  ErrorWidget widget("Error de prueba", Qt::PlainText);
  QVERIFY(widget.findChildren<QWidget*>().size() >= 1);
}

void TestUxQt::testConfigurationDialogShowsPathsAndReloadsSettings() {
  SettingsStore settings;
  settings.setValue(QStringLiteral("testing/dialog"), QStringLiteral("before reload"));

  ConfigurationDialog dialog;
  const auto* filePath = dialog.findChild<QLabel*>(QStringLiteral("configurationFilePath"));
  const auto* profilesPath = dialog.findChild<QLabel*>(QStringLiteral("configurationProfilesPath"));
  const auto* directoryStatus = dialog.findChild<QLabel*>(QStringLiteral("configurationDirectoryStatus"));
  auto* contents = dialog.findChild<QPlainTextEdit*>(QStringLiteral("configurationContents"));
  auto* copyButton = dialog.findChild<QPushButton*>(QStringLiteral("configurationCopyPathButton"));
  auto* reloadButton = dialog.findChild<QPushButton*>(QStringLiteral("configurationReloadButton"));

  QVERIFY(filePath);
  QVERIFY(filePath->text().contains(SettingsStore::configFilePath()));
  QVERIFY(profilesPath);
  QVERIFY(profilesPath->text().contains(SettingsStore::profilesDirectory()));
  QVERIFY(directoryStatus);
  QVERIFY(directoryStatus->text().contains(QStringLiteral("writable")));
  QVERIFY(contents);
  QVERIFY(contents->isReadOnly());
  QVERIFY(contents->toPlainText().contains(QStringLiteral("before reload")));
  QVERIFY(copyButton);
  QVERIFY(reloadButton);

  QTest::mouseClick(copyButton, Qt::LeftButton);
  QCOMPARE(QApplication::clipboard()->text(), SettingsStore::configFilePath());

  settings.setValue(QStringLiteral("testing/dialog"), QStringLiteral("after reload"));
  QTest::mouseClick(reloadButton, Qt::LeftButton);
  QVERIFY(contents->toPlainText().contains(QStringLiteral("after reload")));
}

void TestUxQt::testSingleColumnSettingUpdatesButtonState() {
  QToolButton button;
  button.setCheckable(true);
  QVERIFY(!button.isChecked());

  setSingleColumnViewButtonState(button, true);
  QVERIFY(button.isChecked());

  setSingleColumnViewButtonState(button, false);
  QVERIFY(!button.isChecked());
}
