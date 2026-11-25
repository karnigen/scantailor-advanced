// Copyright (C) 2026 ScanTailor Advanced contributors
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "tst_mainwindow.h"
#include "MainWindow.h"

#include <ImageInfo.h>
#include <ImageTransformation.h>
#include <PageSequence.h>
#include <ProcessingTaskQueue.h>
#include <ProjectPages.h>
#include <StageSequence.h>
#include <WorkerThreadPool.h>
#include <filters/page_layout/Params.h>
#include <filters/select_content/CacheDrivenTask.h>
#include <filters/select_content/Dependencies.h>
#include <filters/select_content/Settings.h>

#include <QDomDocument>
#include <QFile>
#include <QFileInfo>
#include <QImage>
#include <QSemaphore>
#include <QTemporaryDir>
#include <QTimer>
#include <QtTest/QtTest>
#include <functional>
#include <stdexcept>

#include "CompositeCacheDrivenTask.h"
#include "ContentBoxPropagator.h"

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

class BlockingTask : public BackgroundTask {
 public:
  BlockingTask(QSemaphore& started, QSemaphore& release, std::function<void()> update)
      : BackgroundTask(BATCH), m_started(started), m_release(release), m_update(std::move(update)) {}

  FilterResultPtr operator()() override {
    m_update();
    m_started.release();
    if (!m_release.tryAcquire(1, 10000)) {
      throw std::runtime_error("test worker timed out");
    }
    throwIfCancelled();
    return nullptr;
  }

  FilterResultPtr exceptionResult(const QString&) override { return nullptr; }

 private:
  QSemaphore& m_started;
  QSemaphore& m_release;
  std::function<void()> m_update;
};
}  // namespace

void TestMainWindowAsync::cancelledBatchDefersSaveAndKeepsLatestFilter() {
  QTemporaryDir directory;
  QVERIFY(directory.isValid());
  const QString imagePath = directory.filePath("page.png");
  QImage image(100, 120, QImage::Format_RGB32);
  image.fill(Qt::white);
  QVERIFY(image.save(imagePath));
  const ImageId imageId(imagePath, 0);
  const auto pages = std::make_shared<ProjectPages>(
      std::vector<ImageInfo>{ImageInfo(imageId, ImageMetadata(image.size(), Dpi(300, 300)), 1, false, false)},
      Qt::LeftToRight);
  const QString projectPath = directory.filePath("project.ScanTailor");
  QSemaphore started;
  QSemaphore release;
  MainWindow window;
  window.switchToNewProject(pages, directory.path(), projectPath);
  QTRY_VERIFY(window.m_workerThreadPool->isIdle());

  const int margins = window.m_stages->selectContentFilterIdx() + 1;
  const int output = window.m_stages->outputFilterIdx();
  window.filterList->selectRow(margins);
  QTRY_VERIFY(!window.m_waitingForCancelledTasks);
  QTRY_VERIFY(window.m_workerThreadPool->isIdle());

  const PageInfo page = pages->toPageSequence(PAGE_VIEW).pageAt(0);
  const ImageTransformation xform(QRectF(0, 0, 100, 120), Dpi(300, 300));
  const QRectF pageRect(0, 0, 100, 120);
  const QRectF oldContent(10, 10, 80, 100);
  const QRectF newContent(15, 15, 70, 90);
  const select_content::Dependencies deps(xform.resultingPreCropArea(), MODE_MANUAL, MODE_MANUAL, false);
  const auto contentSettings = std::make_shared<select_content::Settings>();
  const select_content::Params updated(newContent, QSizeF(10, 10), pageRect, deps, MODE_MANUAL, MODE_MANUAL, false);
  window.m_stages->pageLayoutFilter()->setContentBox(page.id(), xform, pageRect, oldContent);
  const auto cache = std::make_shared<SelectContentCache>(contentSettings, xform);
  window.m_contentBoxPropagator = std::make_unique<ContentBoxPropagator>(window.m_stages->pageLayoutFilter(), cache);

  const auto task = std::make_shared<BlockingTask>(
      started, release, [contentSettings, page, updated]() { contentSettings->setPageParams(page.id(), updated); });
  window.m_batchQueue = std::make_unique<ProcessingTaskQueue>();
  window.m_batchQueue->addProcessingTask(page, task);
  QCOMPARE(window.m_batchQueue->takeForProcessing(), task);
  window.m_workerThreadPool->submitTask(task);
  QTRY_VERIFY(started.available() == 1);
  window.stopBatchProcessing();
  QVERIFY(window.m_waitingForCancelledTasks);

  window.filterList->selectRow(output);
  window.filterList->selectRow(margins);
  window.filterList->selectRow(output);
  QCOMPARE(window.m_pendingFilter, output);
  window.saveProjectTriggered();
  QVERIFY(!QFileInfo::exists(projectPath));
  bool responsive = false;
  QTimer::singleShot(0, [&responsive]() { responsive = true; });
  QTRY_VERIFY(responsive);
  QVERIFY(window.m_waitingForCancelledTasks);

  release.release();
  QTRY_VERIFY(!window.m_waitingForCancelledTasks);
  QCOMPARE(window.m_curFilter, output);
  QVERIFY(QFileInfo::exists(projectPath));
  QFile file(projectPath);
  QVERIFY(file.open(QIODevice::ReadOnly));
  QDomDocument document;
  QVERIFY(document.setContent(&file));
  const QDomElement savedPage = document.elementsByTagName("page-layout").at(0).toElement().firstChildElement("page");
  QVERIFY(!savedPage.isNull());
  const page_layout::Params saved(savedPage.firstChildElement("params"));
  QCOMPARE(saved.contentRect(), newContent);
}
