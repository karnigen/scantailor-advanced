// SPDX-FileCopyrightText: 2026 project contributors
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <QObject>

class TestMainWindowAsync : public QObject {
  Q_OBJECT
 private slots:
  void cancelledBatchDefersSaveAndKeepsLatestFilter();
  void cancelledBatchReplenishesWorkersAfterPageRemoval();
  void cancelledBatchStopsWhenAllPagesAreRemoved();
  void deferredSavesPreserveOrder_data();
  void deferredSavesPreserveOrder();
  void keyboardShortcutsShowConciseStageNames();
  void filterShortcutsUseCtrlNumberRow();
  void saveProjectPromptUsesActionButtons();
  void reloadProjectConfirmsDiscardWithoutOfferingSave();
  void closeProjectSaveCreatesHistoryBackup();
  void compareFilesDetectsSameSizeChanges();
  void windowTitleShowsUnsavedProjectChanges();
  void supportedImageFormatsHelpShowsOutputAndCompression();
  void pdfToImagesMenuRequiresNoLoadedProject();
  void splitModeDialogOptionsFollowSelectedLayout();
  void applyAlignmentCopiesSelectedSettingsIndependently();
  void applySourceDpiCopiesSelectedAxesIndependently();
  void filterDockWidthRestoresFromConfiguration();
  void sourceDpiChangesPersistInMargins();
  void marginsGuideContextMenuIsAvailableAcrossView();
  void sourceImageLocationDetection();
  void relinkingDialogShowsInitialPath();
  void relinkingBreadcrumbClearsHoverAfterClick();
  void relinkingDialogUsesConfigDirectoryAsBase();
  void relinkingDialogDisablesConfigPathForOutput();
  void relinkingStatusColorsFollowPalette();
  void relinkingCommonImageRootDoesNotChangeOutputDirectory();
  void relinkingCommonImageRootLeavesExternalOutputAlone();
  void relinkingDialogGroupsPathsInDirectoryTree();
  void relinkingRetainsMultipleMappings();
  void missingOutputDirectoryPrompt_data();
  void missingOutputDirectoryPrompt();
  void thumbnailWheelNavigationUsesShiftPageStride();
  void fixDpiDialogListsAndUpdatesPageDpi();
  void fixDpiDialogSortsProjectDpiNumerically();
  void fixDpiDialogRequiresDpiForEveryPage();
  void updateProjectDpiMetadataMarksProjectModified();
  void fixDpiDialogPersistsProjectDpiOnAccept();
  void processingIndicatorPreservesUnderlyingContent();
  void batchProcessingBenchmark();
  void stageSwitchBenchmark();
};
