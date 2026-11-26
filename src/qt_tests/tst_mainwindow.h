// Copyright (C) 2026 ScanTailor Advanced contributors
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef TST_MAINWINDOW_H
#define TST_MAINWINDOW_H

#include <QObject>

class TestMainWindowAsync : public QObject {
  Q_OBJECT
 private slots:
  void cancelledBatchDefersSaveAndKeepsLatestFilter();
  void deferredSavesPreserveOrder_data();
  void deferredSavesPreserveOrder();
  void keyboardShortcutsShowConciseStageNames();
  void supportedImageFormatsHelpShowsOutputAndCompression();
  void applyAlignmentCopiesSelectedSettingsIndependently();
  void applySourceDpiCopiesSelectedAxesIndependently();
  void filterDockWidthRestoresFromConfiguration();
  void sourceDpiChangesPersistInMargins();
  void sourceImageLocationDetection();
  void relinkingDialogShowsInitialPath();
  void relinkingDialogUsesConfigDirectoryAsBase();
  void relinkingDialogDisablesConfigPathForOutput();
  void relinkingStatusColorsFollowPalette();
  void relinkingCommonImageRootDoesNotChangeOutputDirectory();
  void relinkingCommonImageRootLeavesExternalOutputAlone();
  void relinkingDialogGroupsPathsInDirectoryTree();
  void relinkingRetainsMultipleMappings();
  void missingOutputDirectoryPrompt_data();
  void missingOutputDirectoryPrompt();
  void batchProcessingBenchmark();
  void stageSwitchBenchmark();
};

#endif
