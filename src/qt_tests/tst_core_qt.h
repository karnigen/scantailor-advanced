// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <QObject>
#include <QString>

class TestCoreQt : public QObject {
  Q_OBJECT
 private slots:
  void testUnitsToString();
  void testUnitsFromString();
  void testUnitsToLocalizedString();
  void testPageSizePresetsApplyPhysicalDimensions();
  void testPageSizePresetNameMatchesEitherOrientation();
  void testPageDimensionsCommitAfterEditing();
  void testUtilsDoubleToString();
  void testSmartFilenameOrdering();
  void testSignalSpy();
  void testBackgroundExecutorContainsWorkerAndResultExceptions();
  void testProcessingTaskQueueReturnsPageInfoForTask();
  void testWorkerThreadPoolReturnsFailureResult();
  void testWorkerThreadPoolCompletesWhenFailureResultIsUnavailable();
  void testWorkerThreadPoolCompletesUnexpectedCancellation();
  void testWorkerThreadPoolReportsEmptyResult();
  void testCancelledTasksFinishAsynchronously();
  void testContentBoxPropagationPersistsGeometry();
  void testCancelledContentUpdateIsPropagatedBeforeSave();
  void testPerSpreadReadingDirectionPersistsAndAffectsOutputNames();
  void testImageViewResizePreservesZoomFocalPoint();
};

class Emitter : public QObject {
  Q_OBJECT
 public:
  explicit Emitter(QObject* parent = nullptr) : QObject(parent) {}
  void doEmit(const QString& value) { emit valueChanged(value); }
 signals:
  void valueChanged(const QString& value);
};
