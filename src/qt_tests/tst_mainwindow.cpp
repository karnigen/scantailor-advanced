// Copyright (C) 2026 ScanTailor Advanced contributors
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "tst_mainwindow.h"
#include "MainWindow.h"

#include <ApplicationSettings.h>
#include <FilterOptionsWidget.h>
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

#include <QAbstractButton>
#include <QApplication>
#include <QComboBox>
#include <QDialog>
#include <QDir>
#include <QDomDocument>
#include <QElapsedTimer>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QImage>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QSemaphore>
#include <QTemporaryDir>
#include <QTextBrowser>
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

class AutoSaveSetting {
 public:
  AutoSaveSetting() : m_original(ApplicationSettings::getInstance().isAutoSaveProjectEnabled()) {
    ApplicationSettings::getInstance().setAutoSaveProjectEnabled(true);
  }

  ~AutoSaveSetting() { ApplicationSettings::getInstance().setAutoSaveProjectEnabled(m_original); }

 private:
  bool m_original;
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

void TestMainWindowAsync::deferredSavesPreserveOrder_data() {
  QTest::addColumn<bool>("saveFirst");
  QTest::addColumn<bool>("cancelSaveAs");
  QTest::addColumn<bool>("saveDuringDialog");
  QTest::newRow("save-as-then-save") << false << false << false;
  QTest::newRow("cancel-save-as-then-save") << false << true << false;
  QTest::newRow("save-then-save-as") << true << false << false;
  QTest::newRow("save-during-save-as-dialog") << false << false << true;
}

void TestMainWindowAsync::deferredSavesPreserveOrder() {
  QFETCH(bool, saveFirst);
  QFETCH(bool, cancelSaveAs);
  QFETCH(bool, saveDuringDialog);

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
  const QString originalPath = directory.filePath("original.ScanTailor");
  const QString copyPath = directory.filePath("copy.ScanTailor");
  QSemaphore started;
  QSemaphore release;
  AutoSaveSetting autoSaveSetting;
  MainWindow window;
  window.switchToNewProject(pages, directory.path(), originalPath);
  QTRY_VERIFY(window.m_workerThreadPool->isIdle());
  QVERIFY(window.saveProjectInteractive());

  QFile original(originalPath);
  QVERIFY(original.open(QIODevice::Append));
  QCOMPARE(original.write("\n<!-- original marker -->\n"), qint64(26));
  original.close();
  QVERIFY(original.open(QIODevice::ReadOnly));
  const QByteArray originalContents = original.readAll();
  original.close();

  const auto task = std::make_shared<BlockingTask>(started, release, []() {});
  window.m_batchQueue = std::make_unique<ProcessingTaskQueue>();
  window.m_batchQueue->addProcessingTask(pages->toPageSequence(PAGE_VIEW).pageAt(0), task);
  QCOMPARE(window.m_batchQueue->takeForProcessing(), task);
  window.m_workerThreadPool->submitTask(task);
  QTRY_VERIFY(started.available() == 1);
  window.stopBatchProcessing();
  QVERIFY(window.m_waitingForCancelledTasks);

  window.autoSaveProject();
  if (saveFirst) {
    window.saveProjectTriggered();
  }
  window.saveProjectAsTriggered();
  window.autoSaveProject();
  if (!saveFirst && !saveDuringDialog) {
    window.saveProjectTriggered();
  }

  bool dialogSeen = false;
  QTimer dialogTimer;
  connect(&dialogTimer, &QTimer::timeout, &window, [&]() {
    auto* dialog = qobject_cast<QFileDialog*>(QApplication::activeModalWidget());
    if (!dialog) {
      return;
    }
    dialogTimer.stop();
    dialogSeen = true;
    if (saveDuringDialog) {
      window.saveProjectTriggered();
    }
    if (cancelSaveAs) {
      dialog->reject();
    } else {
      dialog->selectFile(copyPath);
      QMetaObject::invokeMethod(dialog, "accept", Qt::QueuedConnection);
    }
  });
  dialogTimer.start(10);
  QTimer::singleShot(5000, &window, []() {
    if (auto* dialog = qobject_cast<QFileDialog*>(QApplication::activeModalWidget())) {
      dialog->reject();
    }
  });
  release.release();
  QTRY_VERIFY(!window.m_waitingForCancelledTasks);
  QVERIFY(dialogSeen);
  QCOMPARE(QFileInfo::exists(copyPath), !cancelSaveAs);
  QVERIFY(original.open(QIODevice::ReadOnly));
  QCOMPARE(original.readAll() == originalContents, !saveFirst && !cancelSaveAs);
}

void TestMainWindowAsync::missingOutputDirectoryPrompt_data() {
  QTest::addColumn<bool>("createOutputDirectory");
  QTest::newRow("create-directory") << true;
  QTest::newRow("decline-directory") << false;
}

void TestMainWindowAsync::missingOutputDirectoryPrompt() {
  QFETCH(bool, createOutputDirectory);

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
  const QString outputDirectory = directory.filePath("output");
  QVERIFY(QDir().mkpath(outputDirectory));
  const QString projectPath = directory.filePath("project.ScanTailor");

  {
    MainWindow creator;
    creator.switchToNewProject(pages, outputDirectory, projectPath);
    QTRY_VERIFY(creator.m_workerThreadPool->isIdle());
    QVERIFY(creator.saveProjectInteractive());
  }
  QVERIFY(QDir(outputDirectory).removeRecursively());

  MainWindow window;
  bool promptSeen = false;
  QString unexpectedDialog;
  QTimer dialogWatcher;
  connect(&dialogWatcher, &QTimer::timeout, &window, [&promptSeen, &unexpectedDialog, createOutputDirectory]() {
    QWidget* activeDialog = QApplication::activeModalWidget();
    if (!activeDialog) {
      return;
    }
    if (auto* messageBox = qobject_cast<QMessageBox*>(activeDialog)) {
      if (messageBox->windowTitle() == QObject::tr("Create Directory?")) {
        promptSeen = true;
        messageBox->button(createOutputDirectory ? QMessageBox::Yes : QMessageBox::No)->click();
        return;
      }
      unexpectedDialog = messageBox->windowTitle() + ": " + messageBox->text();
      messageBox->done(QMessageBox::Ok);
    } else if (auto* dialog = qobject_cast<QDialog*>(activeDialog)) {
      unexpectedDialog = dialog->windowTitle();
      dialog->reject();
    }
  });
  dialogWatcher.start(10);

  window.openProject(projectPath);
  QTRY_VERIFY_WITH_TIMEOUT(promptSeen, 5000);
  QCOMPARE(QDir(outputDirectory).exists(), createOutputDirectory);
  QVERIFY2(unexpectedDialog.isEmpty(), qPrintable(unexpectedDialog));
  if (createOutputDirectory) {
    QTRY_VERIFY_WITH_TIMEOUT(window.isProjectLoaded(), 10000);
    QTRY_VERIFY_WITH_TIMEOUT(window.m_workerThreadPool->isIdle(), 10000);
  } else {
    QVERIFY(!window.isProjectLoaded());
  }
  dialogWatcher.stop();
}

void TestMainWindowAsync::keyboardShortcutsShowConciseStageNames() {
  MainWindow window;
  QString shortcutHelp;
  QTimer::singleShot(0, &window, [&shortcutHelp]() {
    auto* dialog = qobject_cast<QDialog*>(QApplication::activeModalWidget());
    if (!dialog) {
      return;
    }
    if (auto* browser = dialog->findChild<QTextBrowser*>()) {
      shortcutHelp = browser->toPlainText();
    }
    dialog->reject();
  });

  QVERIFY(QMetaObject::invokeMethod(&window, "showKeyboardShortcuts", Qt::DirectConnection));
  QVERIFY(shortcutHelp.contains("Fix Orientation"));
  QVERIFY(!shortcutHelp.contains("Open the manual Fix Orientation stage."));
}

void TestMainWindowAsync::supportedImageFormatsHelpShowsOutputAndCompression() {
  MainWindow window;
  QVERIFY(QMetaObject::invokeMethod(&window, "showSupportedImageFormats", Qt::DirectConnection));

  auto* dialog = window.findChild<QDialog*>(QStringLiteral("supportedImageFormatsDialog"));
  QVERIFY(dialog);
  auto* browser = dialog->findChild<QTextBrowser*>();
  QVERIFY(browser);

  const QString help = browser->toPlainText();
  QVERIFY(help.contains("TIFF (.tif, .tiff)"));
  QVERIFY(help.contains("TIFF (.tif)"));
  QVERIFY(help.contains("B&W Compression"));
  QVERIFY(help.contains("Color Compression"));
  QVERIFY(help.contains("CCITT G4"));
  QVERIFY(help.contains("JPEG"));
  dialog->close();
}

void TestMainWindowAsync::sourceDpiChangesPersistInMargins() {
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
  MainWindow window;
  window.switchToNewProject(pages, directory.path());
  QTRY_VERIFY_WITH_TIMEOUT(window.m_workerThreadPool->isIdle(), 10000);
  const int pageLayoutFilterIdx = window.m_stages->pageLayoutFilterIdx();
  window.filterList->selectRow(pageLayoutFilterIdx);
  QTRY_COMPARE_WITH_TIMEOUT(window.m_curFilter, pageLayoutFilterIdx, 10000);
  QTRY_VERIFY_WITH_TIMEOUT(!window.m_optionsWidget.isNull(), 10000);
  QTRY_VERIFY_WITH_TIMEOUT(window.m_workerThreadPool->isIdle(), 10000);

  auto* dpiCombo = window.m_optionsWidget->findChild<QComboBox*>("sourceDpiCombo");
  auto* xDpi = window.m_optionsWidget->findChild<QLineEdit*>("sourceXDpi");
  auto* yDpi = window.m_optionsWidget->findChild<QLineEdit*>("sourceYDpi");
  auto* dpiLabel = window.m_optionsWidget->findChild<QLabel*>("sourceDpiLabel");
  auto* dpiValuesLayout = window.m_optionsWidget->findChild<QHBoxLayout*>("sourceDpiValuesLayout");
  QVERIFY(dpiCombo);
  QVERIFY(xDpi);
  QVERIFY(yDpi);
  QVERIFY(dpiLabel);
  QVERIFY(dpiValuesLayout);
  QCOMPARE(dpiLabel->text(), QStringLiteral("sDPI"));
  QCOMPARE(dpiValuesLayout->spacing(), 0);

  dpiCombo->setCurrentIndex(2);
  QVERIFY(QMetaObject::invokeMethod(window.m_optionsWidget.data(), "sourceDpiComboActivated", Qt::DirectConnection,
                                    Q_ARG(int, 2)));
  QCoreApplication::processEvents();
  QTRY_COMPARE_WITH_TIMEOUT(window.m_thumbSequence->toPageSequence().pageAt(size_t(0)).metadata().dpi(), Dpi(400, 400),
                            10000);
  QTRY_VERIFY_WITH_TIMEOUT(window.m_workerThreadPool->isIdle(), 10000);
  QCOMPARE(xDpi->text(), QStringLiteral("400"));
  QCOMPARE(yDpi->text(), QStringLiteral("400"));
  QCOMPARE(window.m_pages->toPageSequence(PAGE_VIEW).pageAt(size_t(0)).metadata().dpi(), Dpi(400, 400));

  xDpi->setText(QStringLiteral("600"));
  yDpi->setText(QStringLiteral("400"));
  QVERIFY(QMetaObject::invokeMethod(window.m_optionsWidget.data(), "sourceDpiEditingFinished", Qt::DirectConnection));
  QCoreApplication::processEvents();
  QTRY_COMPARE_WITH_TIMEOUT(window.m_thumbSequence->toPageSequence().pageAt(size_t(0)).metadata().dpi(), Dpi(600, 400),
                            10000);
  QTRY_VERIFY_WITH_TIMEOUT(window.m_workerThreadPool->isIdle(), 10000);
  QCOMPARE(xDpi->text(), QStringLiteral("600"));
  QCOMPARE(yDpi->text(), QStringLiteral("400"));
  QCOMPARE(window.m_pages->toPageSequence(PAGE_VIEW).pageAt(size_t(0)).metadata().dpi(), Dpi(600, 400));
}

void TestMainWindowAsync::batchProcessingBenchmark() {
  const QString projectPath = qEnvironmentVariable("SCANTAILOR_BATCH_BENCH_PROJECT");
  if (projectPath.isEmpty()) {
    QSKIP("Set SCANTAILOR_BATCH_BENCH_PROJECT to run the opt-in batch benchmark.");
  }
  QVERIFY2(QFileInfo(projectPath).isFile(), qPrintable(QString("Project file does not exist: %1").arg(projectPath)));

  const QString stage = qEnvironmentVariable("SCANTAILOR_BATCH_BENCH_STAGE", "page-layout");
  QString batchWarning;
  MainWindow window(QFileInfo(projectPath).absolutePath());
  int filterIndex = -1;
  if (stage == "fix-orientation") {
    filterIndex = window.m_stages->fixOrientationFilterIdx();
  } else if (stage == "select-content") {
    filterIndex = window.m_stages->selectContentFilterIdx();
  } else if (stage == "page-layout") {
    filterIndex = window.m_stages->pageLayoutFilterIdx();
  } else if (stage == "output") {
    filterIndex = window.m_stages->outputFilterIdx();
  } else {
    QFAIL("SCANTAILOR_BATCH_BENCH_STAGE must be fix-orientation, select-content, page-layout, or output.");
  }

  QTimer dialogWatcher;
  connect(&dialogWatcher, &QTimer::timeout, &window, [&batchWarning]() {
    auto* dialog = qobject_cast<QMessageBox*>(QApplication::activeModalWidget());
    if (!dialog) {
      return;
    }
    if (dialog->windowTitle() == QObject::tr("Batch processing")) {
      batchWarning = dialog->text();
    }
    dialog->accept();
  });
  dialogWatcher.start(25);

  window.openProject(projectPath);
  QTRY_VERIFY_WITH_TIMEOUT(window.isProjectLoaded(), 120000);
  QTRY_VERIFY_WITH_TIMEOUT(window.m_workerThreadPool->isIdle(), 120000);

  window.filterList->selectRow(filterIndex);
  QTRY_COMPARE_WITH_TIMEOUT(window.m_curFilter, filterIndex, 120000);
  QTRY_VERIFY_WITH_TIMEOUT(!window.m_waitingForCancelledTasks, 120000);
  QTRY_VERIFY_WITH_TIMEOUT(window.m_workerThreadPool->isIdle(), 120000);

  QElapsedTimer timer;
  timer.start();
  window.startBatchProcessing();
  QVERIFY(window.isBatchProcessingInProgress());
  QTRY_VERIFY_WITH_TIMEOUT(!window.isBatchProcessingInProgress(), 1800000);
  const qint64 elapsedMs = timer.elapsed();

  QTRY_VERIFY_WITH_TIMEOUT(window.m_workerThreadPool->isIdle(), 120000);
  QTest::qWait(50);
  dialogWatcher.stop();
  QVERIFY2(batchWarning.isEmpty(), qPrintable(batchWarning));

  qInfo().noquote() << QStringLiteral("PR196_BATCH_BENCH stage=%1 pages=%2 elapsed_ms=%3")
                           .arg(stage)
                           .arg(static_cast<qulonglong>(window.allPages().numPages()))
                           .arg(elapsedMs);
}

void TestMainWindowAsync::stageSwitchBenchmark() {
  const QString projectPath = qEnvironmentVariable("SCANTAILOR_BATCH_BENCH_PROJECT");
  if (projectPath.isEmpty()) {
    QSKIP("Set SCANTAILOR_BATCH_BENCH_PROJECT to run the opt-in stage-switch benchmark.");
  }
  QVERIFY2(QFileInfo(projectPath).isFile(), qPrintable(QString("Project file does not exist: %1").arg(projectPath)));

  MainWindow window(QFileInfo(projectPath).absolutePath());
  window.openProject(projectPath);
  QTRY_VERIFY_WITH_TIMEOUT(window.isProjectLoaded(), 120000);
  QTRY_VERIFY_WITH_TIMEOUT(window.m_workerThreadPool->isIdle(), 120000);
  window.show();
  QTest::qWait(100);

  const int fixOrientation = window.m_stages->fixOrientationFilterIdx();
  const int pageLayout = window.m_stages->pageLayoutFilterIdx();
  window.filterList->selectRow(pageLayout);
  QTRY_COMPARE_WITH_TIMEOUT(window.m_curFilter, pageLayout, 120000);
  QTRY_VERIFY_WITH_TIMEOUT(window.m_workerThreadPool->isIdle(), 120000);
  window.filterList->selectRow(fixOrientation);
  QTRY_COMPARE_WITH_TIMEOUT(window.m_curFilter, fixOrientation, 120000);
  QTRY_VERIFY_WITH_TIMEOUT(window.m_workerThreadPool->isIdle(), 120000);
  QTest::qWait(100);

  for (int iteration = 0; iteration < 6; ++iteration) {
    const int target = (iteration % 2 == 0) ? pageLayout : fixOrientation;
    QElapsedTimer timer;
    timer.start();
    window.filterList->selectRow(target);
    QTRY_COMPARE_WITH_TIMEOUT(window.m_curFilter, target, 120000);
    QTRY_VERIFY_WITH_TIMEOUT(!window.m_waitingForCancelledTasks, 120000);
    QTRY_VERIFY_WITH_TIMEOUT(window.m_workerThreadPool->isIdle(), 120000);
    window.repaint();
    QApplication::processEvents();
    const qint64 elapsedUs = timer.nsecsElapsed() / 1000;
    qInfo().noquote() << QStringLiteral("PR196_STAGE_SWITCH_BENCH target=%1 pages=%2 elapsed_us=%3")
                             .arg(target == pageLayout ? "page-layout" : "fix-orientation")
                             .arg(static_cast<qulonglong>(window.allPages().numPages()))
                             .arg(elapsedUs);
  }
}
