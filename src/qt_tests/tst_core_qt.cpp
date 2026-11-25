// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "tst_core_qt.h"

#include <SmartFilenameOrdering.h>
#include <Units.h>
#include <filters/page_layout/Filter.h>
#include <filters/page_layout/Params.h>
#include <filters/select_content/CacheDrivenTask.h>
#include <filters/select_content/Dependencies.h>
#include <filters/select_content/Settings.h>
#include <foundation/Utils.h>

#include <QDomDocument>
#include <QFile>
#include <QFileInfo>
#include <QSignalSpy>
#include <QString>
#include <QTemporaryDir>
#include <QtTest/QtTest>
#include <stdexcept>
#include <utility>

#include "AbstractCommand.h"
#include "BackgroundExecutor.h"
#include "BackgroundTask.h"
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

}  // namespace

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
  const OutputFileNameGenerator output(std::make_shared<FileNameDisambiguator>(), directory.path(), Qt::LeftToRight);
  const ProjectWriter writer(pages, SelectedPage(), output);
  const QString projectPath = directory.filePath("project.scan");
  QString error;
  QVERIFY2(writer.write(projectPath, {filter}, &error), qPrintable(error));
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

  contentSettings->clearPageParams(pageIds[1]);
  ContentBoxPropagator(filter, cache).propagate(*pages);
  QDomDocument updatedDocument;
  const QDomElement updatedFilter = filter->saveSettings(writer, updatedDocument);
  const QDomNodeList updatedPages = updatedFilter.elementsByTagName("page");
  QCOMPARE(updatedPages.size(), 2);
  const page_layout::Params unchanged(updatedPages.at(0).firstChildElement("params"));
  const page_layout::Params unavailable(updatedPages.at(1).firstChildElement("params"));
  QCOMPARE(unchanged.contentRect(), contentRects[0]);
  QVERIFY(unavailable.contentRect().isEmpty());
  QVERIFY(unavailable.contentSizeMM().isEmpty());
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
  const PageInfo page(PageId(ImageId(QStringLiteral("/tmp/page.tif")), PageId::RIGHT_PAGE), ImageMetadata(), 1, false,
                      false);
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
