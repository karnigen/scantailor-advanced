// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#include "tst_core_qt.h"

#include <Dpi.h>
#include <PageSizePresets.h>
#include <PathUtils.h>
#include <SmartFilenameOrdering.h>
#include <Units.h>
#include <UnitsConverter.h>
#include <filters/page_layout/Filter.h>
#include <filters/page_layout/Params.h>
#include <filters/select_content/CacheDrivenTask.h>
#include <filters/select_content/Dependencies.h>
#include <filters/select_content/OptionsWidget.h>
#include <filters/select_content/Settings.h>
#include <foundation/Utils.h>

#include <QComboBox>
#include <QDomDocument>
#include <QDoubleSpinBox>
#include <QFile>
#include <QFileInfo>
#include <QSemaphore>
#include <QSignalSpy>
#include <QString>
#include <QTemporaryDir>
#include <QtTest/QtTest>
#include <stdexcept>
#include <utility>

#include "AbstractCommand.h"
#include "BackgroundExecutor.h"
#include "BackgroundTask.h"
#include "BasicImageView.h"
#include "CompositeCacheDrivenTask.h"
#include "ContentBoxPropagator.h"
#include "FileNameDisambiguator.h"
#include "FilterResult.h"
#include "ImageInfo.h"
#include "ImageTransformation.h"
#include "OutputFileNameGenerator.h"
#include "PageSelectionAccessor.h"
#include "ProcessingTaskQueue.h"
#include "ProjectPages.h"
#include "ProjectWriter.h"
#include "SelectedPage.h"
#include "WorkerThreadPool.h"

namespace {

class SelectContentCache : public CompositeCacheDrivenTask {
 public:
  SelectContentCache(std::shared_ptr<select_content::Settings> settings, const ImageTransformation& xform)
      : m_task(std::move(settings), nullptr), m_xform(xform) {}

  void process(const PageInfo& pageInfo, AbstractFilterDataCollector* collector) override {
    m_task.process(pageInfo, collector, m_xform);
  }

 private:
  select_content::CacheDrivenTask m_task;
  ImageTransformation m_xform;
};

struct AsyncTaskState {
  bool failed = false;
  bool resultEventReceived = false;
  QString errorMessage;
};

class ResultCallback : public AbstractCommand<void> {
 public:
  explicit ResultCallback(const bool throwOnRun) : m_throwOnRun(throwOnRun) {}

  void operator()() override {
    if (m_throwOnRun) {
      throw std::runtime_error("result callback failed");
    }
  }

 private:
  bool m_throwOnRun;
};

class FailingExecutorTask : public BackgroundExecutor::Task {
 public:
  enum class FailurePoint { Worker, Unknown, ResultCallback };

  FailingExecutorTask(AsyncTaskState& state, const FailurePoint failurePoint)
      : m_state(state), m_failurePoint(failurePoint) {}

  BackgroundExecutor::TaskResultPtr operator()() override {
    if (m_failurePoint == FailurePoint::Worker) {
      throw std::runtime_error("worker failed");
    }
    if (m_failurePoint == FailurePoint::Unknown) {
      throw 42;
    }
    return std::make_shared<ResultCallback>(true);
  }

  void handleException(const QString& message) override {
    m_state.failed = true;
    m_state.errorMessage = message;
  }

 private:
  AsyncTaskState& m_state;
  FailurePoint m_failurePoint;
};

class FailingFilterResult : public FilterResult {
 public:
  FailingFilterResult(AsyncTaskState& state, QString message) : m_state(state), m_message(std::move(message)) {}

  void updateUI(FilterUiInterface*) override {
    m_state.failed = true;
    m_state.errorMessage = m_message;
  }

  std::shared_ptr<AbstractFilter> filter() override { return nullptr; }

 private:
  AsyncTaskState& m_state;
  QString m_message;
};

class FailingBackgroundTask : public BackgroundTask {
 public:
  enum class ResultFactoryFailure { None, Null, Throw };

  explicit FailingBackgroundTask(AsyncTaskState& state,
                                 const ResultFactoryFailure resultFactoryFailure = ResultFactoryFailure::None)
      : BackgroundTask(INTERACTIVE), m_state(state), m_resultFactoryFailure(resultFactoryFailure) {}

  FilterResultPtr operator()() override { throw std::runtime_error("page processing failed"); }

  FilterResultPtr exceptionResult(const QString& message) override {
    if (m_resultFactoryFailure == ResultFactoryFailure::Null) {
      return nullptr;
    }
    if (m_resultFactoryFailure == ResultFactoryFailure::Throw) {
      throw std::runtime_error("error result creation failed");
    }
    return std::make_shared<FailingFilterResult>(m_state, message);
  }

 private:
  AsyncTaskState& m_state;
  ResultFactoryFailure m_resultFactoryFailure;
};

class UnexpectedlyCancelledBackgroundTask : public BackgroundTask {
 public:
  UnexpectedlyCancelledBackgroundTask() : BackgroundTask(INTERACTIVE) {}

  FilterResultPtr operator()() override { throw CancelledException(); }

  FilterResultPtr exceptionResult(const QString&) override { return nullptr; }
};

class EmptyResultBackgroundTask : public BackgroundTask {
 public:
  EmptyResultBackgroundTask() : BackgroundTask(BATCH) {}

  FilterResultPtr operator()() override { return nullptr; }

  FilterResultPtr exceptionResult(const QString&) override { return nullptr; }
};

class InterruptedContentTask : public BackgroundTask {
 public:
  InterruptedContentTask(std::shared_ptr<select_content::Settings> settings,
                         PageId pageId,
                         select_content::Params params,
                         QSemaphore& started,
                         QSemaphore& release)
      : BackgroundTask(BATCH),
        m_settings(std::move(settings)),
        m_pageId(std::move(pageId)),
        m_params(std::move(params)),
        m_started(started),
        m_release(release) {}

  FilterResultPtr operator()() override {
    m_settings->setPageParams(m_pageId, m_params);
    m_started.release();
    if (!m_release.tryAcquire(1, 10000)) {
      throw std::runtime_error("test worker timed out");
    }
    throwIfCancelled();
    return nullptr;
  }

  FilterResultPtr exceptionResult(const QString&) override { return nullptr; }

 private:
  std::shared_ptr<select_content::Settings> m_settings;
  PageId m_pageId;
  select_content::Params m_params;
  QSemaphore& m_started;
  QSemaphore& m_release;
};

}  // namespace

void TestCoreQt::testCancelledTasksFinishAsynchronously() {
  const auto settings = std::make_shared<select_content::Settings>();
  const PageId pageId(ImageId("/page.tif", 0), PageId::SINGLE_PAGE);
  const ImageTransformation xform(QRectF(0, 0, 200, 300), Dpi(300, 300));
  const select_content::Dependencies deps(xform.resultingPreCropArea(), MODE_MANUAL, MODE_MANUAL, false);
  const select_content::Params params(QRectF(10, 10, 100, 100), QSizeF(10, 10), QRectF(0, 0, 200, 300), deps,
                                      MODE_MANUAL, MODE_MANUAL, false);
  QSemaphore started;
  QSemaphore release;
  const auto task = std::make_shared<InterruptedContentTask>(settings, pageId, params, started, release);
  WorkerThreadPool workerPool;
  QSignalSpy idleSpy(&workerPool, &WorkerThreadPool::idle);
  workerPool.submitTask(task);
  QTRY_VERIFY(started.available() == 1);
  task->cancel();

  AsyncTaskState state;
  const auto alreadyCancelled = std::make_shared<FailingBackgroundTask>(state);
  alreadyCancelled->cancel();
  workerPool.submitTask(alreadyCancelled);
  bool eventLoopResponsive = false;
  QTimer::singleShot(0, [&eventLoopResponsive]() { eventLoopResponsive = true; });
  QTRY_VERIFY(eventLoopResponsive);
  QVERIFY(!workerPool.isIdle());
  QCOMPARE(idleSpy.count(), 0);

  release.release();
  QTRY_VERIFY(workerPool.isIdle());
  QCOMPARE(idleSpy.count(), 1);
  QVERIFY(!state.failed);
}

void TestCoreQt::testCancelledContentUpdateIsPropagatedBeforeSave() {
  const ImageMetadata metadata(QSize(200, 300), Dpi(300, 300));
  const ImageId imageId("/page.tif", 0);
  const PageId pageId(imageId, PageId::SINGLE_PAGE);
  const auto pages = std::make_shared<ProjectPages>(
      std::vector<ImageInfo>{ImageInfo(imageId, metadata, 1, false, false)}, Qt::LeftToRight);
  const auto contentSettings = std::make_shared<select_content::Settings>();
  const ImageTransformation xform(QRectF(0, 0, 200, 300), Dpi(300, 300));
  const select_content::Dependencies deps(xform.resultingPreCropArea(), MODE_MANUAL, MODE_MANUAL, false);
  const QRectF pageRect(5, 6, 180, 280);
  const QRectF oldContent(15, 20, 140, 230);
  const QRectF newContent(25, 30, 130, 210);
  const auto filter = std::make_shared<page_layout::Filter>(pages, PageSelectionAccessor(nullptr));
  filter->setContentBox(pageId, xform, pageRect, oldContent);
  const auto cache = std::make_shared<SelectContentCache>(contentSettings, xform);
  ContentBoxPropagator propagator(filter, cache);
  QSemaphore started;
  QSemaphore release;
  const auto task = std::make_shared<InterruptedContentTask>(
      contentSettings, pageId,
      select_content::Params(newContent, QSizeF(10, 10), pageRect, deps, MODE_MANUAL, MODE_MANUAL, false), started,
      release);
  WorkerThreadPool workerPool;
  connect(&workerPool, &WorkerThreadPool::idle, this, [&]() { propagator.propagate(*pages); });
  workerPool.submitTask(task);
  QTRY_VERIFY(started.available() == 1);
  task->cancel();
  release.release();
  QTRY_VERIFY(workerPool.isIdle());

  const QTemporaryDir directory;
  QVERIFY(directory.isValid());
  const OutputFileNameGenerator output(std::make_shared<FileNameDisambiguator>(),
                                       core::toFileSystemPath(directory.path()), Qt::LeftToRight);
  const ProjectWriter writer(pages, SelectedPage(), output);
  const QString projectPath = directory.filePath("project.scan");
  QString error;
  QVERIFY2(writer.write(core::toFileSystemPath(projectPath), {filter}, &error), qPrintable(error));
  QFile file(projectPath);
  QVERIFY(file.open(QIODevice::ReadOnly));
  QDomDocument document;
  QVERIFY(document.setContent(&file));
  const QDomElement page = document.elementsByTagName("page-layout").at(0).toElement().firstChildElement("page");
  QVERIFY(!page.isNull());
  const page_layout::Params saved(page.firstChildElement("params"));
  QCOMPARE(saved.pageRect(), pageRect);
  QCOMPARE(saved.contentRect(), newContent);
}

void TestCoreQt::testContentBoxPropagationPersistsGeometry() {
  const ImageMetadata metadata(QSize(200, 300), Dpi(300, 300));
  const ImageId firstId("/first.tif", 0);
  const ImageId secondId("/second.tif", 0);
  const auto pages
      = std::make_shared<ProjectPages>(std::vector<ImageInfo>{ImageInfo(firstId, metadata, 1, false, false),
                                                              ImageInfo(secondId, metadata, 1, false, false)},
                                       Qt::LeftToRight);
  const auto contentSettings = std::make_shared<select_content::Settings>();
  const ImageTransformation xform(QRectF(0, 0, 200, 300), Dpi(300, 300));
  const select_content::Dependencies deps(xform.resultingPreCropArea(), MODE_MANUAL, MODE_MANUAL, false);
  const QRectF pageRect(5, 6, 180, 280);
  const QRectF contentRects[] = {QRectF(15, 20, 140, 230), QRectF(25, 30, 130, 210)};
  const PageId pageIds[] = {PageId(firstId, PageId::SINGLE_PAGE), PageId(secondId, PageId::SINGLE_PAGE)};
  for (int i = 0; i < 2; ++i) {
    contentSettings->setPageParams(pageIds[i], select_content::Params(contentRects[i], QSizeF(10, 10), pageRect, deps,
                                                                      MODE_MANUAL, MODE_MANUAL, false));
  }

  const auto filter = std::make_shared<page_layout::Filter>(pages, PageSelectionAccessor(nullptr));
  const auto cache = std::make_shared<SelectContentCache>(contentSettings, xform);
  ContentBoxPropagator(filter, cache).propagate(*pages);

  const QTemporaryDir directory;
  QVERIFY(directory.isValid());
  const OutputFileNameGenerator output(std::make_shared<FileNameDisambiguator>(),
                                       core::toFileSystemPath(directory.path()), Qt::LeftToRight);
  const ProjectWriter writer(pages, SelectedPage(), output);
  const QString projectPath = directory.filePath("project.scan");
  QString error;
  QVERIFY2(writer.write(core::toFileSystemPath(projectPath), {filter}, &error), qPrintable(error));
  QFile file(projectPath);
  QVERIFY(file.open(QIODevice::ReadOnly));
  QDomDocument document;
  QVERIFY(document.setContent(&file));
  const QDomNodeList savedPages = document.elementsByTagName("page-layout").at(0).toElement().elementsByTagName("page");
  QCOMPARE(savedPages.size(), 2);
  for (int i = 0; i < 2; ++i) {
    const QDomElement paramsElement = savedPages.at(i).firstChildElement("params");
    QVERIFY(!paramsElement.isNull());
    const page_layout::Params params(paramsElement);
    QCOMPARE(params.pageRect(), pageRect);
    QCOMPARE(params.contentRect(), contentRects[i]);
    QVERIFY(!params.contentSizeMM().isEmpty());
  }

  const QRectF changedContent(35, 40, 100, 180);
  for (const PageId& pageId : pageIds) {
    contentSettings->setPageParams(pageId, select_content::Params(changedContent, QSizeF(10, 10), pageRect, deps,
                                                                  MODE_MANUAL, MODE_MANUAL, false));
  }
  ContentBoxPropagator(filter, cache).propagateImages(*pages, {firstId});
  QDomDocument partialDocument;
  const QDomNodeList partiallyUpdated = filter->saveSettings(writer, partialDocument).elementsByTagName("page");
  QCOMPARE(partiallyUpdated.size(), 2);
  QCOMPARE(page_layout::Params(partiallyUpdated.at(0).firstChildElement("params")).contentRect(), changedContent);
  QCOMPARE(page_layout::Params(partiallyUpdated.at(1).firstChildElement("params")).contentRect(), contentRects[1]);

  contentSettings->clearPageParams(pageIds[1]);
  ContentBoxPropagator(filter, cache).propagate(*pages);
  QDomDocument updatedDocument;
  const QDomElement updatedFilter = filter->saveSettings(writer, updatedDocument);
  const QDomNodeList updatedPages = updatedFilter.elementsByTagName("page");
  QCOMPARE(updatedPages.size(), 2);
  const page_layout::Params unchanged(updatedPages.at(0).firstChildElement("params"));
  const page_layout::Params unavailable(updatedPages.at(1).firstChildElement("params"));
  QCOMPARE(unchanged.contentRect(), changedContent);
  QVERIFY(unavailable.contentRect().isEmpty());
  QVERIFY(unavailable.contentSizeMM().isEmpty());
}

void TestCoreQt::testImageViewResizePreservesZoomFocalPoint() {
  QImage image(1600, 1200, QImage::Format_RGB32);
  image.fill(Qt::white);

  BasicImageView view(image);
  view.resize(900, 700);
  view.show();
  QTRY_VERIFY(view.maximumViewportSize().width() > 0);

  const QSize oldViewportSize = view.maximumViewportSize();
  const QPointF oldCenter(oldViewportSize.width() / 2.0, oldViewportSize.height() / 2.0);
  const QPointF focalOffset(120.0, -80.0);
  view.setWidgetFocalPointWithoutMoving(oldCenter + focalOffset);
  view.setZoomLevel(2.0);

  view.resize(700, 700);
  QTRY_VERIFY(view.maximumViewportSize().width() != oldViewportSize.width());

  const QSize newViewportSize = view.maximumViewportSize();
  const QPointF newCenter(newViewportSize.width() / 2.0, newViewportSize.height() / 2.0);
  QCOMPARE(view.getWidgetFocalPoint() - newCenter, focalOffset);
}

void TestCoreQt::testUnitsToString() {
  QCOMPARE(unitsToString(PIXELS), QString("px"));
  QCOMPARE(unitsToString(MILLIMETRES), QString("mm"));
  QCOMPARE(unitsToString(CENTIMETRES), QString("cm"));
  QCOMPARE(unitsToString(INCHES), QString("in"));
}

void TestCoreQt::testUnitsFromString() {
  QCOMPARE(unitsFromString(QString("px")), PIXELS);
  QCOMPARE(unitsFromString(QString("mm")), MILLIMETRES);
  QCOMPARE(unitsFromString(QString("cm")), CENTIMETRES);
  QCOMPARE(unitsFromString(QString("in")), INCHES);
  QCOMPARE(unitsFromString(QString("unknown")), MILLIMETRES);
}

void TestCoreQt::testUnitsToLocalizedString() {
  QString px = unitsToLocalizedString(PIXELS);
  QString mm = unitsToLocalizedString(MILLIMETRES);
  QVERIFY(!px.isEmpty());
  QVERIFY(!mm.isEmpty());
}

void TestCoreQt::testPageSizePresetsApplyPhysicalDimensions() {
  const Dpi dpi(300, 300);
  const PageId pageId(ImageId(core::toFileSystemPath(QStringLiteral("/tmp/page.tif"))), PageId::SINGLE_PAGE);
  const PageInfo page(pageId, ImageMetadata(QSize(1000, 1200), dpi), 1, false, false);
  select_content::OptionsWidget widget(std::make_shared<select_content::Settings>(), PageSelectionAccessor(nullptr));
  widget.preUpdateUI(page);

  select_content::OptionsWidget::UiData uiData;
  uiData.setPageRect(QRectF(10, 20, 800, 1000));
  uiData.setPageDetectionMode(MODE_MANUAL);
  widget.postUpdateUI(uiData);

  auto* pageSizeCombo = widget.findChild<QComboBox*>(QStringLiteral("pageSizeCombo"));
  auto* widthSpinBox = widget.findChild<QDoubleSpinBox*>(QStringLiteral("widthSpinBox"));
  QVERIFY(pageSizeCombo);
  QVERIFY(widthSpinBox);

  const int a4PortraitIndex = pageSizeCombo->findText(QStringLiteral("A4 (portrait)"));
  QVERIFY(a4PortraitIndex > 0);

  QSignalSpy pageRectSpy(&widget, &select_content::OptionsWidget::pageRectChangedLocally);
  pageSizeCombo->setCurrentIndex(a4PortraitIndex);
  QCOMPARE(pageRectSpy.count(), 1);

  const QRectF selectedPageRect = pageRectSpy.takeFirst().at(0).toRectF();
  double expectedWidth = 210.0;
  double expectedHeight = 297.0;
  UnitsConverter(dpi).convert(expectedWidth, expectedHeight, MILLIMETRES, PIXELS);
  QVERIFY(std::abs(selectedPageRect.width() - expectedWidth) < 0.2);
  QVERIFY(std::abs(selectedPageRect.height() - expectedHeight) < 0.2);

  const int a4LandscapeIndex = pageSizeCombo->findText(QStringLiteral("A4 (landscape)"));
  QVERIFY(a4LandscapeIndex > 0);
  pageSizeCombo->setCurrentIndex(a4LandscapeIndex);
  QCOMPARE(pageRectSpy.count(), 1);
  const QRectF landscapePageRect = pageRectSpy.takeFirst().at(0).toRectF();
  expectedWidth = 297.0;
  expectedHeight = 210.0;
  UnitsConverter(dpi).convert(expectedWidth, expectedHeight, MILLIMETRES, PIXELS);
  QVERIFY(std::abs(landscapePageRect.width() - expectedWidth) < 0.2);
  QVERIFY(std::abs(landscapePageRect.height() - expectedHeight) < 0.2);

  widthSpinBox->setValue(widthSpinBox->value() + 1.0);
  QCOMPARE(pageSizeCombo->currentIndex(), 0);
}

void TestCoreQt::testPageSizePresetNameMatchesEitherOrientation() {
  QCOMPARE(page_sizes::matchingName(QSizeF(210.0, 297.0)), QStringLiteral("A4"));
  QCOMPARE(page_sizes::matchingName(QSizeF(297.0, 210.0)), QStringLiteral("A4"));
  QVERIFY(page_sizes::matchingName(QSizeF(210.0, 300.0)).isEmpty());
}

void TestCoreQt::testPageDimensionsCommitAfterEditing() {
  const Dpi dpi(300, 300);
  const PageId pageId(ImageId(core::toFileSystemPath(QStringLiteral("/tmp/page.tif"))), PageId::SINGLE_PAGE);
  const PageInfo page(pageId, ImageMetadata(QSize(1000, 1200), dpi), 1, false, false);
  select_content::OptionsWidget widget(std::make_shared<select_content::Settings>(), PageSelectionAccessor(nullptr));
  widget.preUpdateUI(page);

  select_content::OptionsWidget::UiData uiData;
  uiData.setPageRect(QRectF(10, 20, 800, 1000));
  uiData.setPageDetectionMode(MODE_MANUAL);
  widget.postUpdateUI(uiData);

  auto* widthSpinBox = widget.findChild<QDoubleSpinBox*>(QStringLiteral("widthSpinBox"));
  auto* heightSpinBox = widget.findChild<QDoubleSpinBox*>(QStringLiteral("heightSpinBox"));
  QVERIFY(widthSpinBox);
  QVERIFY(heightSpinBox);
  QVERIFY(!widthSpinBox->keyboardTracking());
  QVERIFY(!heightSpinBox->keyboardTracking());

  QSignalSpy pageRectSpy(&widget, &select_content::OptionsWidget::pageRectChangedLocally);
  const auto replaceValue = [&pageRectSpy](QDoubleSpinBox* spinBox) {
    spinBox->setFocus();
    spinBox->selectAll();
    QTest::keyClick(spinBox, Qt::Key_Backspace);
    QTest::keyClicks(spinBox, QStringLiteral("123"));
    QCOMPARE(pageRectSpy.count(), 0);
    QVERIFY(spinBox->text().contains(QStringLiteral("123")));

    QTest::keyClick(spinBox, Qt::Key_Return);
    QCOMPARE(pageRectSpy.count(), 1);
    pageRectSpy.clear();
  };
  replaceValue(widthSpinBox);
  replaceValue(heightSpinBox);
}

void TestCoreQt::testUtilsDoubleToString() {
  QCOMPARE(foundation::Utils::doubleToString(0.0), QString("0"));
  QVERIFY(foundation::Utils::doubleToString(123.456).contains("123"));
  QVERIFY(foundation::Utils::doubleToString(-42.5).startsWith("-"));
}

void TestCoreQt::testSmartFilenameOrdering() {
  const SmartFilenameOrdering less;
  const QFileInfo a("/etc/1.png");
  const QFileInfo b("/etc/2.png");
  QVERIFY(less(a, b));
  QVERIFY(!less(b, a));
  QVERIFY(!less(a, a));

  const QFileInfo c("/etc/a_0002.png");
  const QFileInfo d("/etc/a_1.png");
  QVERIFY(less(d, c));
}

void TestCoreQt::testSignalSpy() {
  Emitter obj;
  QSignalSpy spy(&obj, &Emitter::valueChanged);
  QVERIFY(spy.isValid());
  QCOMPARE(spy.count(), 0);

  obj.doEmit(QString("hello"));
  QCOMPARE(spy.count(), 1);
  QCOMPARE(spy.at(0).at(0).toString(), QString("hello"));

  obj.doEmit(QString("world"));
  QCOMPARE(spy.count(), 2);
  QCOMPARE(spy.at(1).at(0).toString(), QString("world"));
}

void TestCoreQt::testBackgroundExecutorContainsWorkerAndResultExceptions() {
  const FailingExecutorTask::FailurePoint failurePoints[] = {
      FailingExecutorTask::FailurePoint::Worker,
      FailingExecutorTask::FailurePoint::Unknown,
      FailingExecutorTask::FailurePoint::ResultCallback,
  };
  const QString expectedMessages[] = {
      QString("worker failed"),
      QString(),
      QString("result callback failed"),
  };

  for (int i = 0; i < 3; ++i) {
    AsyncTaskState state;
    {
      BackgroundExecutor executor;
      executor.enqueueTask(std::make_shared<FailingExecutorTask>(state, failurePoints[i]));
      QTRY_VERIFY(state.failed);
    }
    QCOMPARE(state.errorMessage, expectedMessages[i]);
  }
}

void TestCoreQt::testProcessingTaskQueueReturnsPageInfoForTask() {
  AsyncTaskState state;
  const BackgroundTaskPtr task = std::make_shared<FailingBackgroundTask>(state);
  const PageInfo page(PageId(ImageId(core::toFileSystemPath(QStringLiteral("/tmp/page.tif"))), PageId::RIGHT_PAGE),
                      ImageMetadata(), 1, false, false);
  ProcessingTaskQueue queue;
  queue.addProcessingTask(page, task);
  QCOMPARE(queue.takeForProcessing(), task);

  const PageInfo retrievedPage = queue.pageInfoForTask(task);
  QCOMPARE(retrievedPage.imageId().filePath(), page.imageId().filePath());
  QCOMPARE(retrievedPage.id().subPage(), PageId::RIGHT_PAGE);

  queue.processingFinished(task);
  QVERIFY(queue.pageInfoForTask(task).isNull());
}

void TestCoreQt::testWorkerThreadPoolReturnsFailureResult() {
  AsyncTaskState state;
  WorkerThreadPool workerPool;
  connect(&workerPool, &WorkerThreadPool::taskResult, this,
          [&state](const BackgroundTaskPtr&, const FilterResultPtr& result, const QString&) {
            QVERIFY(result);
            result->updateUI(nullptr);
            state.resultEventReceived = true;
          });

  workerPool.submitTask(std::make_shared<FailingBackgroundTask>(state));
  QTRY_VERIFY(state.resultEventReceived);
  QVERIFY(state.failed);
  QCOMPARE(state.errorMessage, QString("page processing failed"));
}

void TestCoreQt::testWorkerThreadPoolCompletesWhenFailureResultIsUnavailable() {
  const FailingBackgroundTask::ResultFactoryFailure failures[] = {
      FailingBackgroundTask::ResultFactoryFailure::Null,
      FailingBackgroundTask::ResultFactoryFailure::Throw,
  };
  for (const auto failure : failures) {
    AsyncTaskState state;
    WorkerThreadPool workerPool;
    connect(&workerPool, &WorkerThreadPool::taskResult, this,
            [&state](const BackgroundTaskPtr&, const FilterResultPtr& result, const QString& errorMessage) {
              state.resultEventReceived = true;
              QVERIFY(!result);
              state.errorMessage = errorMessage;
            });

    workerPool.submitTask(std::make_shared<FailingBackgroundTask>(state, failure));
    QTRY_VERIFY(state.resultEventReceived);
    QCOMPARE(state.errorMessage, QString("page processing failed"));
  }
}

void TestCoreQt::testWorkerThreadPoolCompletesUnexpectedCancellation() {
  WorkerThreadPool workerPool;
  bool resultEventReceived = false;
  QString errorMessage;
  connect(&workerPool, &WorkerThreadPool::taskResult, this,
          [&resultEventReceived, &errorMessage](const BackgroundTaskPtr&, const FilterResultPtr& result,
                                                const QString& message) {
            QVERIFY(!result);
            resultEventReceived = true;
            errorMessage = message;
          });

  workerPool.submitTask(std::make_shared<UnexpectedlyCancelledBackgroundTask>());
  QTRY_VERIFY(resultEventReceived);
  QCOMPARE(errorMessage, QStringLiteral("Background task was cancelled unexpectedly."));
}

void TestCoreQt::testWorkerThreadPoolReportsEmptyResult() {
  WorkerThreadPool workerPool;
  bool resultEventReceived = false;
  connect(&workerPool, &WorkerThreadPool::taskResult, this,
          [&resultEventReceived](const BackgroundTaskPtr&, const FilterResultPtr& result, const QString&) {
            QVERIFY(!result);
            resultEventReceived = true;
          });
  workerPool.submitTask(std::make_shared<EmptyResultBackgroundTask>());
  QTRY_VERIFY(resultEventReceived);
  QVERIFY(workerPool.isIdle());
}
