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
#include <core/SettingsStore.h>
#include <filters/page_layout/Params.h>
#include <filters/select_content/CacheDrivenTask.h>
#include <filters/select_content/Dependencies.h>
#include <filters/select_content/Settings.h>

#include "RelinkablePathVisualization.h"
#include "RelinkingDialog.h"

#include <QAbstractButton>
#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
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
#include <QPushButton>
#include <QRadioButton>
#include <QSemaphore>
#include <QTemporaryDir>
#include <QTextBrowser>
#include <QTextCursor>
#include <QTextDocument>
#include <QTextFormat>
#include <QTimer>
#include <QToolButton>
#include <QTreeView>
#include <QWheelEvent>
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

class ByteArraySettingRestore {
 public:
  ByteArraySettingRestore(SettingsStore& settings, QString key)
      : m_settings(settings),
        m_key(std::move(key)),
        m_hadValue(settings.contains(m_key)),
        m_originalValue(settings.byteArrayValue(m_key)) {}

  ~ByteArraySettingRestore() {
    if (m_hadValue) {
      m_settings.setValue(m_key, m_originalValue);
    } else {
      m_settings.remove(m_key);
    }
  }

 private:
  SettingsStore& m_settings;
  QString m_key;
  bool m_hadValue;
  QByteArray m_originalValue;
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

void TestMainWindowAsync::sourceImageLocationDetection() {
  QTemporaryDir directory;
  QVERIFY(directory.isValid());

  const QString projectDirectory = directory.filePath("project");
  const QString nestedDirectory = QDir(projectDirectory).filePath("images/nested");
  const QString siblingDirectory = directory.filePath("project-copy");
  QVERIFY(QDir().mkpath(nestedDirectory));
  QVERIFY(QDir().mkpath(siblingDirectory));

  const QString nestedImage = QDir(nestedDirectory).filePath("page.png");
  const QString siblingImage = QDir(siblingDirectory).filePath("page.png");
  for (const QString& path : {nestedImage, siblingImage}) {
    QFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly));
  }

  QVERIFY(MainWindow::isPathWithinDirectory(nestedImage, projectDirectory));
  QVERIFY(MainWindow::isPathWithinDirectory(QDir(projectDirectory).filePath("missing.png"), projectDirectory));
  QVERIFY(!MainWindow::isPathWithinDirectory(siblingImage, projectDirectory));

  const QString commonRoot = RelinkingModel::findCommonImageRoot({nestedImage, siblingImage});
  QCOMPARE(commonRoot, QDir::cleanPath(directory.path()));
  QVERIFY(MainWindow::isPathWithinDirectory(QDir(commonRoot).filePath("out"), commonRoot));
  QVERIFY(!MainWindow::isPathWithinDirectory(QDir(directory.path()).absoluteFilePath("../outside/out"), commonRoot));
}

void TestMainWindowAsync::relinkingDialogShowsInitialPath() {
  QTemporaryDir directory;
  QVERIFY(directory.isValid());

  RelinkingDialog dialog(directory.filePath("project.ScanTailor"));
  const QString imagePath = directory.filePath("input/page.png");
  dialog.pathCollector()(RelinkablePath(imagePath, RelinkablePath::File));
  dialog.selectFirstPath();
  dialog.show();

  auto* pathVisualization = dialog.findChild<RelinkablePathVisualization*>("pathVisualization");
  QVERIFY(pathVisualization);
  QTRY_VERIFY(pathVisualization->isVisible());
  const QList<QPushButton*> pathButtons = pathVisualization->findChildren<QPushButton*>();
  QVERIFY(!pathButtons.isEmpty());
  QCOMPARE(pathButtons.last()->text(), QStringLiteral("page.png"));
}

void TestMainWindowAsync::relinkingBreadcrumbClearsHoverAfterClick() {
  QTemporaryDir directory;
  QVERIFY(directory.isValid());

  RelinkablePathVisualization breadcrumb;
  breadcrumb.resize(800, 50);
  breadcrumb.setPath(RelinkablePath(directory.path(), RelinkablePath::Dir), true);
  breadcrumb.show();
  const QList<QPushButton*> buttons = breadcrumb.findChildren<QPushButton*>();
  QVERIFY(!buttons.isEmpty());
  QPushButton* lastButton = buttons.last();
  QTRY_VERIFY(lastButton->isVisible());

  const QPoint buttonCenter(lastButton->rect().center());
  QTest::mouseMove(lastButton, buttonCenter);
  QTRY_VERIFY(lastButton->property("forceHighlight").toBool());
  QTest::mouseClick(lastButton, Qt::LeftButton, Qt::NoModifier, buttonCenter);
  QTRY_VERIFY(!lastButton->property("forceHighlight").toBool());
  QVERIFY(!lastButton->property("stickHighlight").toBool());

  QTest::mouseMove(lastButton, buttonCenter + QPoint(1, 0));
  QTRY_VERIFY(lastButton->property("forceHighlight").toBool());
}

void TestMainWindowAsync::relinkingDialogUsesConfigDirectoryAsBase() {
  QTemporaryDir directory;
  QVERIFY(directory.isValid());
  const QString configDirectory = directory.filePath("project");
  const QString sourceDirectory = directory.filePath("source");
  QVERIFY(QDir().mkpath(configDirectory));
  QVERIFY(QDir().mkpath(sourceDirectory));
  const QString imagePath = QDir(sourceDirectory).filePath("page.png");
  QFile imageFile(imagePath);
  QVERIFY(imageFile.open(QIODevice::WriteOnly));
  imageFile.close();

  RelinkingDialog dialog(QDir(configDirectory).filePath("project.ScanTailor"));
  dialog.pathCollector()(RelinkablePath(imagePath, RelinkablePath::File));
  dialog.addCommonImageRoot();
  dialog.selectFirstPath();
  auto* useConfigPathButton = dialog.findChild<QPushButton*>("useConfigPathButton");
  QVERIFY(useConfigPathButton);
  QVERIFY(useConfigPathButton->isEnabled());
  QVERIFY(useConfigPathButton->property("imageRootOutsideProject").toBool());
  useConfigPathButton->click();
  QVERIFY(!useConfigPathButton->property("imageRootOutsideProject").toBool());

  auto* buttonBox = dialog.findChild<QDialogButtonBox*>("buttonBox");
  QVERIFY(buttonBox);
  buttonBox->button(QDialogButtonBox::Ok)->click();

  const std::shared_ptr<AbstractRelinker> relinker = dialog.relinker();
  QCOMPARE(relinker->substitutionPathFor(RelinkablePath(imagePath, RelinkablePath::File)),
           QDir(configDirectory).filePath("page.png"));
}

void TestMainWindowAsync::relinkingDialogDisablesConfigPathForOutput() {
  QTemporaryDir directory;
  QVERIFY(directory.isValid());

  RelinkingDialog dialog(directory.filePath("project.ScanTailor"));
  dialog.addOutputPath(RelinkablePath(directory.filePath("out"), RelinkablePath::Dir));
  dialog.selectFirstPath();

  auto* useConfigPathButton = dialog.findChild<QPushButton*>("useConfigPathButton");
  QVERIFY(useConfigPathButton);
  QVERIFY(!useConfigPathButton->isEnabled());
}

void TestMainWindowAsync::relinkingStatusColorsFollowPalette() {
  const QPalette originalPalette = QApplication::palette();
  QPalette palette = originalPalette;

  palette.setColor(QPalette::Window, Qt::white);
  QApplication::setPalette(palette);
  const QColor lightExistsColor = RelinkingModel::foregroundColorForStatus(RelinkingModel::Exists);
  const QColor lightMissingColor = RelinkingModel::foregroundColorForStatus(RelinkingModel::Missing);

  palette.setColor(QPalette::Window, QColor(40, 40, 40));
  QApplication::setPalette(palette);
  const QColor darkExistsColor = RelinkingModel::foregroundColorForStatus(RelinkingModel::Exists);
  const QColor darkMissingColor = RelinkingModel::foregroundColorForStatus(RelinkingModel::Missing);

  QApplication::setPalette(originalPalette);
  QVERIFY(lightExistsColor.lightness() < QColor(Qt::white).lightness());
  QVERIFY(darkExistsColor.lightness() > QColor(40, 40, 40).lightness());
  QVERIFY(lightMissingColor.lightness() < QColor(Qt::white).lightness());
  QVERIFY(darkMissingColor.lightness() > QColor(40, 40, 40).lightness());
  QVERIFY(lightExistsColor != darkExistsColor);
  QVERIFY(lightMissingColor != darkMissingColor);
}

void TestMainWindowAsync::relinkingRetainsMultipleMappings() {
  RelinkingModel model;
  const RelinkablePath firstPath(QStringLiteral("/original-a/page.png"), RelinkablePath::File);
  const RelinkablePath secondPath(QStringLiteral("/original-b/page.png"), RelinkablePath::File);
  model.addPath(firstPath);
  model.addPath(secondPath);
  model.replacePrefix(firstPath.normalizedPath(), QStringLiteral("/replacement-a/page.png"), RelinkablePath::File,
                      RelinkingModel::ImageFile);
  model.commitChanges();
  model.replacePrefix(secondPath.normalizedPath(), QStringLiteral("/replacement-b/page.png"), RelinkablePath::File,
                      RelinkingModel::ImageFile);
  model.commitChanges();

  const std::shared_ptr<AbstractRelinker> relinker = model.relinker();
  QCOMPARE(relinker->substitutionPathFor(firstPath), QStringLiteral("/replacement-a/page.png"));
  QCOMPARE(relinker->substitutionPathFor(secondPath), QStringLiteral("/replacement-b/page.png"));
}

void TestMainWindowAsync::relinkingCommonImageRootDoesNotChangeOutputDirectory() {
  QTemporaryDir directory;
  QVERIFY(directory.isValid());

  const QString imageRoot = directory.filePath("dataset");
  const QString configDirectory = directory.filePath("project");
  const QString outputDirectory = QDir(imageRoot).filePath("out");
  const RelinkablePath firstImage(QDir(imageRoot).filePath("img1/page1.png"), RelinkablePath::File);
  const RelinkablePath secondImage(QDir(imageRoot).filePath("img2/page2.png"), RelinkablePath::File);
  QVERIFY(QDir().mkpath(QFileInfo(firstImage.normalizedPath()).absolutePath()));
  QVERIFY(QDir().mkpath(QFileInfo(secondImage.normalizedPath()).absolutePath()));
  QVERIFY(QDir().mkpath(outputDirectory));
  QVERIFY(QDir().mkpath(configDirectory));

  RelinkingModel model;
  model.addPath(firstImage);
  model.addPath(secondImage);
  model.addOutputPath(RelinkablePath(outputDirectory, RelinkablePath::Dir));
  model.addCommonImageRoot();

  QString commonRootPath;
  for (int row = 0; row < model.rowCount(); ++row) {
    const QModelIndex index(model.index(row, 0));
    if (index.data(RelinkingModel::EntryKindRole).toInt() == RelinkingModel::CommonImageRoot) {
      commonRootPath = index.data(RelinkingModel::UncommittedPathRole).toString();
    }
  }
  QCOMPARE(commonRootPath, QDir::cleanPath(imageRoot));
  const RelinkingModel::Summary beforeRelink = model.summary();
  QCOMPARE(beforeRelink.imagesInsideRoot, 2);
  QCOMPARE(beforeRelink.imagesOutsideRoot, 0);
  QVERIFY(beforeRelink.outputInsideRoot);
  for (int row = 0; row < model.rowCount(); ++row) {
    const QModelIndex index(model.index(row, 0));
    if (index.data(RelinkingModel::EntryKindRole).toInt() == RelinkingModel::ImageFile) {
      QCOMPARE(index.data(Qt::DisplayRole).toString(), QStringLiteral("img1/page1.png"));
      break;
    }
  }

  model.replacePrefix(commonRootPath, configDirectory, RelinkablePath::Dir, RelinkingModel::CommonImageRoot);
  model.commitChanges();

  const std::shared_ptr<AbstractRelinker> relinker = model.relinker();
  QCOMPARE(relinker->substitutionPathFor(firstImage), QDir(configDirectory).filePath("img1/page1.png"));
  QCOMPARE(relinker->substitutionPathFor(secondImage), QDir(configDirectory).filePath("img2/page2.png"));
  QCOMPARE(relinker->substitutionPathFor(RelinkablePath(outputDirectory, RelinkablePath::Dir)),
           QDir(configDirectory).filePath("out"));
}

void TestMainWindowAsync::relinkingCommonImageRootLeavesExternalOutputAlone() {
  QTemporaryDir directory;
  QVERIFY(directory.isValid());

  const QString imageRoot = directory.filePath("dataset");
  const QString replacementRoot = directory.filePath("relocated");
  const QString outputDirectory = directory.filePath("separate-output");
  const RelinkablePath image(QDir(imageRoot).filePath("batch/page.png"), RelinkablePath::File);
  QVERIFY(QDir().mkpath(QFileInfo(image.normalizedPath()).absolutePath()));
  QVERIFY(QDir().mkpath(outputDirectory));
  QVERIFY(QDir().mkpath(replacementRoot));

  RelinkingModel model;
  model.addPath(image);
  model.addOutputPath(RelinkablePath(outputDirectory, RelinkablePath::Dir));
  model.addCommonImageRoot();

  const QString commonRoot = RelinkingModel::findCommonImageRoot({image.normalizedPath()});
  model.replacePrefix(commonRoot, replacementRoot, RelinkablePath::Dir, RelinkingModel::CommonImageRoot);
  model.commitChanges();

  const std::shared_ptr<AbstractRelinker> relinker = model.relinker();
  QCOMPARE(relinker->substitutionPathFor(image), QDir(replacementRoot).filePath("page.png"));
  QCOMPARE(relinker->substitutionPathFor(RelinkablePath(outputDirectory, RelinkablePath::Dir)), outputDirectory);
}

void TestMainWindowAsync::relinkingDialogGroupsPathsInDirectoryTree() {
  QTemporaryDir directory;
  QVERIFY(directory.isValid());

  const QString imageRoot = directory.filePath("dataset");
  const QString projectFile = directory.filePath("project.ScanTailor");
  const QString firstImage = QDir(imageRoot).filePath("dir1/page1.png");
  const QString secondImage = QDir(imageRoot).filePath("dir2/page2.png");
  const QString outputDirectory = QDir(imageRoot).filePath("out");
  QVERIFY(QDir().mkpath(QFileInfo(firstImage).absolutePath()));
  QVERIFY(QDir().mkpath(QFileInfo(secondImage).absolutePath()));
  QVERIFY(QDir().mkpath(outputDirectory));

  RelinkingDialog dialog(projectFile);
  dialog.pathCollector()(RelinkablePath(firstImage, RelinkablePath::File));
  dialog.pathCollector()(RelinkablePath(secondImage, RelinkablePath::File));
  dialog.addOutputPath(RelinkablePath(outputDirectory, RelinkablePath::Dir));
  dialog.addCommonImageRoot();
  dialog.selectFirstPath();

  auto* treeView = dialog.findChild<QTreeView*>("listView");
  QVERIFY(treeView);
  const QModelIndex rootIndex = treeView->model()->index(0, 0);
  QVERIFY(rootIndex.isValid());
  QCOMPARE(rootIndex.data(RelinkingModel::EntryKindRole).toInt(), RelinkingModel::CommonImageRoot);
  QCOMPARE(rootIndex.data(RelinkingModel::UncommittedPathRole).toString(), QDir::cleanPath(imageRoot));
  QCOMPARE(treeView->model()->rowCount(rootIndex), 3);

  const QModelIndex firstChild = treeView->model()->index(0, 0, rootIndex);
  QVERIFY(firstChild.isValid());
  QCOMPARE(firstChild.data(RelinkingModel::EntryKindRole).toInt(), RelinkingModel::OutputDirectory);

  int directoryGroups = 0;
  int outputEntries = 0;
  for (int row = 0; row < treeView->model()->rowCount(rootIndex); ++row) {
    const QModelIndex child = treeView->model()->index(row, 0, rootIndex);
    if (child.data(RelinkingModel::TreeGroupRole).toBool()) {
      ++directoryGroups;
      QCOMPARE(treeView->model()->rowCount(child), 1);
      const QModelIndex imageLeaf = treeView->model()->index(0, 0, child);
      QVERIFY(imageLeaf.data(RelinkingModel::EntryKindRole).isValid());
      QCOMPARE(imageLeaf.data(Qt::DisplayRole).toString(),
               child.data(Qt::DisplayRole).toString() == QStringLiteral("dir1") ? QStringLiteral("page1.png")
                                                                                : QStringLiteral("page2.png"));
    } else if (child.data(RelinkingModel::EntryKindRole).toInt() == RelinkingModel::OutputDirectory) {
      ++outputEntries;
      QCOMPARE(child.data(Qt::DisplayRole).toString(), QStringLiteral("out (output directory)"));
    }
  }
  QCOMPARE(directoryGroups, 2);
  QCOMPARE(outputEntries, 1);
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

void TestMainWindowAsync::thumbnailWheelNavigationUsesShiftPageStride() {
  QTemporaryDir directory;
  QVERIFY(directory.isValid());

  QImage image(100, 120, QImage::Format_RGB32);
  image.fill(Qt::white);
  std::vector<ImageInfo> imageInfos;
  for (int index = 0; index < 25; ++index) {
    const QString imagePath = directory.filePath(QStringLiteral("page%1.png").arg(index));
    QVERIFY(image.save(imagePath));
    imageInfos.emplace_back(ImageId(imagePath, 0), ImageMetadata(image.size(), Dpi(300, 300)), 1, false, false);
  }
  const auto pages = std::make_shared<ProjectPages>(std::move(imageInfos), Qt::LeftToRight);

  MainWindow window;
  window.switchToNewProject(pages, directory.path());
  QTRY_VERIFY_WITH_TIMEOUT(window.m_workerThreadPool->isIdle(), 10000);

  const PageSequence pageSequence = window.m_thumbSequence->toPageSequence();
  const auto sendWheelEvent = [&window](const Qt::KeyboardModifier modifier, const int delta) {
    const QPoint localPosition(10, 10);
    const QPoint globalPosition = window.thumbView->viewport()->mapToGlobal(localPosition);
    QWheelEvent event(localPosition, globalPosition, QPoint(), QPoint(0, delta), Qt::NoButton, modifier,
                      Qt::NoScrollPhase, false);
    QVERIFY(window.eventFilter(window.thumbView->viewport(), &event));
  };

  window.goToPage(pageSequence.pageAt(5).id());
  sendWheelEvent(Qt::ControlModifier, -120);
  QCOMPARE(window.m_thumbSequence->selectionLeader().id(), pageSequence.pageAt(6).id());

  window.goToPage(pageSequence.pageAt(5).id());
  sendWheelEvent(Qt::ShiftModifier, -120);
  QCOMPARE(window.m_thumbSequence->selectionLeader().id(), pageSequence.pageAt(15).id());

  sendWheelEvent(Qt::ShiftModifier, 120);
  QCOMPARE(window.m_thumbSequence->selectionLeader().id(), pageSequence.pageAt(5).id());
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
  QVERIFY(help.contains("kept as TIFF"));
  QVERIFY(help.contains("WebP"));
  QVERIFY(help.contains("multi-page PDF"));
  QVERIFY(help.contains("B&W Compression"));
  QVERIFY(help.contains("Color Compression"));
  QVERIFY(help.contains("CCITT G4"));
  QVERIFY(help.contains("JPEG"));
  dialog->close();
}

void TestMainWindowAsync::applyAlignmentCopiesSelectedSettingsIndependently() {
  QTemporaryDir directory;
  QVERIFY(directory.isValid());

  QImage image(100, 120, QImage::Format_RGB32);
  image.fill(Qt::white);
  const QString firstImagePath = directory.filePath("first.png");
  const QString secondImagePath = directory.filePath("second.png");
  QVERIFY(image.save(firstImagePath));
  QVERIFY(image.save(secondImagePath));

  const ImageId firstImageId(firstImagePath, 0);
  const ImageId secondImageId(secondImagePath, 0);
  const auto pages = std::make_shared<ProjectPages>(
      std::vector<ImageInfo>{ImageInfo(firstImageId, ImageMetadata(image.size(), Dpi(300, 300)), 1, false, false),
                             ImageInfo(secondImageId, ImageMetadata(image.size(), Dpi(300, 300)), 1, false, false)},
      Qt::LeftToRight);
  MainWindow window;
  window.switchToNewProject(pages, directory.path());
  QTRY_VERIFY_WITH_TIMEOUT(window.m_workerThreadPool->isIdle(), 10000);

  const PageSequence pageSequence = window.m_thumbSequence->toPageSequence();
  const PageId firstPage = pageSequence.pageAt(0).id();
  const PageId secondPage = pageSequence.pageAt(1).id();
  const int pageLayoutFilterIdx = window.m_stages->pageLayoutFilterIdx();
  window.filterList->selectRow(pageLayoutFilterIdx);
  QTRY_COMPARE_WITH_TIMEOUT(window.m_curFilter, pageLayoutFilterIdx, 10000);
  QTRY_VERIFY_WITH_TIMEOUT(!window.m_optionsWidget.isNull(), 10000);

  window.goToPage(firstPage);
  QTRY_VERIFY_WITH_TIMEOUT(window.m_workerThreadPool->isIdle(), 10000);
  auto* options = window.m_optionsWidget.data();
  QVERIFY(options);
  QVERIFY(!options->findChild<QPushButton*>("matchSizeToAllBtn"));

  auto* guideHelp = options->findChild<QTextBrowser*>("textBrowser");
  QVERIFY(guideHelp);
  QTextDocument* helpDocument = guideHelp->document();
  QCOMPARE(helpDocument->defaultFont().pointSizeF(), 7.0);
  const QTextCursor staticHelp = helpDocument->find(QStringLiteral("Right-click"));
  const QTextCursor dynamicHelp = helpDocument->find(QStringLiteral("Arrow keys move page content"));
  QVERIFY(!staticHelp.isNull());
  QVERIFY(!dynamicHelp.isNull());
  QVERIFY(!staticHelp.charFormat().hasProperty(QTextFormat::FontPointSize));
  QVERIFY(!dynamicHelp.charFormat().hasProperty(QTextFormat::FontPointSize));

  const QPoint helpPosition(5, 5);
  const QPoint globalHelpPosition = guideHelp->viewport()->mapToGlobal(helpPosition);
  QWheelEvent zoomEvent(helpPosition, globalHelpPosition, QPoint(), QPoint(0, 120), Qt::NoButton, Qt::ControlModifier,
                        Qt::NoScrollPhase, false);
  QApplication::sendEvent(guideHelp->viewport(), &zoomEvent);
  QVERIFY(helpDocument->defaultFont().pointSizeF() > 7.0);

  auto* useForCommonSize = options->findChild<QCheckBox*>("alignWithOthersCB");
  auto* horizontalMode = options->findChild<QComboBox*>("hAlignmentModeCB");
  auto* verticalMode = options->findChild<QComboBox*>("vAlignmentModeCB");
  auto* topRight = options->findChild<QToolButton*>("alignTopRightBtn");
  QVERIFY(useForCommonSize);
  QVERIFY(horizontalMode);
  QVERIFY(verticalMode);
  QVERIFY(topRight);
  useForCommonSize->setChecked(true);
  horizontalMode->setCurrentIndex(1);
  verticalMode->setCurrentIndex(1);
  topRight->click();
  QVERIFY(topRight->isChecked());

  window.goToPage(secondPage);
  QTRY_VERIFY_WITH_TIMEOUT(window.m_workerThreadPool->isIdle(), 10000);
  options = window.m_optionsWidget.data();
  QVERIFY(options);
  useForCommonSize = options->findChild<QCheckBox*>("alignWithOthersCB");
  horizontalMode = options->findChild<QComboBox*>("hAlignmentModeCB");
  verticalMode = options->findChild<QComboBox*>("vAlignmentModeCB");
  auto* bottomLeft = options->findChild<QToolButton*>("alignBottomLeftBtn");
  QVERIFY(useForCommonSize);
  QVERIFY(horizontalMode);
  QVERIFY(verticalMode);
  QVERIFY(bottomLeft);
  useForCommonSize->setChecked(true);
  horizontalMode->setCurrentIndex(1);
  verticalMode->setCurrentIndex(1);
  bottomLeft->click();
  useForCommonSize->setChecked(false);
  QVERIFY(bottomLeft->isChecked());
  QVERIFY(!useForCommonSize->isChecked());

  window.goToPage(firstPage);
  QTRY_VERIFY_WITH_TIMEOUT(window.m_workerThreadPool->isIdle(), 10000);
  options = window.m_optionsWidget.data();
  QVERIFY(options);
  auto* applyButton = options->findChild<QPushButton*>("applyAlignmentBtn");
  QVERIFY(applyButton);
  applyButton->click();
  auto* dialog = options->findChild<QDialog*>();
  QTRY_VERIFY(dialog);
  auto* allPages = dialog->findChild<QRadioButton*>("allPagesRB");
  auto* copyCommonSize = dialog->findChild<QCheckBox*>("copyCommonSizeCB");
  auto* copyAlignment = dialog->findChild<QCheckBox*>("copyAlignmentCB");
  auto* buttonBox = dialog->findChild<QDialogButtonBox*>("buttonBox");
  QVERIFY(allPages);
  QVERIFY(copyCommonSize);
  QVERIFY(copyAlignment);
  QVERIFY(buttonBox);
  allPages->setChecked(true);
  copyCommonSize->setChecked(true);
  copyAlignment->setChecked(false);
  buttonBox->button(QDialogButtonBox::Ok)->click();

  window.goToPage(secondPage);
  QTRY_VERIFY_WITH_TIMEOUT(window.m_workerThreadPool->isIdle(), 10000);
  options = window.m_optionsWidget.data();
  QVERIFY(options);
  useForCommonSize = options->findChild<QCheckBox*>("alignWithOthersCB");
  bottomLeft = options->findChild<QToolButton*>("alignBottomLeftBtn");
  QVERIFY(useForCommonSize);
  QVERIFY(bottomLeft);
  QVERIFY(useForCommonSize->isChecked());
  QVERIFY(bottomLeft->isChecked());

  window.goToPage(firstPage);
  QTRY_VERIFY_WITH_TIMEOUT(window.m_workerThreadPool->isIdle(), 10000);
  options = window.m_optionsWidget.data();
  QVERIFY(options);
  useForCommonSize = options->findChild<QCheckBox*>("alignWithOthersCB");
  QVERIFY(useForCommonSize);
  useForCommonSize->setChecked(false);
  QVERIFY(!useForCommonSize->isChecked());

  applyButton = options->findChild<QPushButton*>("applyAlignmentBtn");
  QVERIFY(applyButton);
  applyButton->click();
  dialog = options->findChild<QDialog*>();
  QTRY_VERIFY(dialog);
  allPages = dialog->findChild<QRadioButton*>("allPagesRB");
  copyCommonSize = dialog->findChild<QCheckBox*>("copyCommonSizeCB");
  copyAlignment = dialog->findChild<QCheckBox*>("copyAlignmentCB");
  buttonBox = dialog->findChild<QDialogButtonBox*>("buttonBox");
  QVERIFY(allPages);
  QVERIFY(copyCommonSize);
  QVERIFY(copyAlignment);
  QVERIFY(buttonBox);
  allPages->setChecked(true);
  copyCommonSize->setChecked(false);
  copyAlignment->setChecked(true);
  buttonBox->button(QDialogButtonBox::Ok)->click();

  window.goToPage(secondPage);
  QTRY_VERIFY_WITH_TIMEOUT(window.m_workerThreadPool->isIdle(), 10000);
  options = window.m_optionsWidget.data();
  QVERIFY(options);
  useForCommonSize = options->findChild<QCheckBox*>("alignWithOthersCB");
  auto* topRightTarget = options->findChild<QToolButton*>("alignTopRightBtn");
  QVERIFY(useForCommonSize);
  QVERIFY(topRightTarget);
  QVERIFY(useForCommonSize->isChecked());
  QVERIFY(topRightTarget->isChecked());
}

void TestMainWindowAsync::applySourceDpiCopiesSelectedAxesIndependently() {
  QTemporaryDir directory;
  QVERIFY(directory.isValid());

  QImage image(100, 120, QImage::Format_RGB32);
  image.fill(Qt::white);
  const QString firstImagePath = directory.filePath("first.png");
  const QString secondImagePath = directory.filePath("second.png");
  QVERIFY(image.save(firstImagePath));
  QVERIFY(image.save(secondImagePath));

  const ImageId firstImageId(firstImagePath, 0);
  const ImageId secondImageId(secondImagePath, 0);
  const auto pages = std::make_shared<ProjectPages>(
      std::vector<ImageInfo>{ImageInfo(firstImageId, ImageMetadata(image.size(), Dpi(300, 300)), 1, false, false),
                             ImageInfo(secondImageId, ImageMetadata(image.size(), Dpi(600, 700)), 1, false, false)},
      Qt::LeftToRight);
  MainWindow window;
  window.switchToNewProject(pages, directory.path());
  QTRY_VERIFY_WITH_TIMEOUT(window.m_workerThreadPool->isIdle(), 10000);

  const PageSequence pageSequence = window.m_thumbSequence->toPageSequence();
  const PageId firstPage = pageSequence.pageAt(0).id();
  const PageId secondPage = pageSequence.pageAt(1).id();
  const int pageLayoutFilterIdx = window.m_stages->pageLayoutFilterIdx();
  window.filterList->selectRow(pageLayoutFilterIdx);
  QTRY_COMPARE_WITH_TIMEOUT(window.m_curFilter, pageLayoutFilterIdx, 10000);
  QTRY_VERIFY_WITH_TIMEOUT(!window.m_optionsWidget.isNull(), 10000);
  window.goToPage(firstPage);
  QTRY_VERIFY_WITH_TIMEOUT(window.m_workerThreadPool->isIdle(), 10000);
  QCOMPARE(pages->toPageSequence(PAGE_VIEW).pageAt(firstPage).metadata().dpi(), Dpi(300, 300));
  QCOMPARE(pages->toPageSequence(PAGE_VIEW).pageAt(secondPage).metadata().dpi(), Dpi(600, 700));

  auto* options = window.m_optionsWidget.data();
  QVERIFY(options);
  QVERIFY(!options->findChild<QPushButton*>("fixDpiBtn"));
  auto* applyButton = options->findChild<QPushButton*>("applySettingsBtn");
  QVERIFY(applyButton);
  applyButton->click();

  auto* dialog = options->findChild<QDialog*>();
  QTRY_VERIFY(dialog);
  auto* allPages = dialog->findChild<QRadioButton*>("allPagesRB");
  auto* applyLeft = dialog->findChild<QCheckBox*>("applyLeftCB");
  auto* applyRight = dialog->findChild<QCheckBox*>("applyRightCB");
  auto* applyTop = dialog->findChild<QCheckBox*>("applyTopCB");
  auto* applyBottom = dialog->findChild<QCheckBox*>("applyBottomCB");
  auto* applyDpiX = dialog->findChild<QCheckBox*>("applyDpiXCB");
  auto* applyDpiY = dialog->findChild<QCheckBox*>("applyDpiYCB");
  auto* buttonBox = dialog->findChild<QDialogButtonBox*>("buttonBox");
  QVERIFY(allPages);
  QVERIFY(applyLeft);
  QVERIFY(applyRight);
  QVERIFY(applyTop);
  QVERIFY(applyBottom);
  QVERIFY(applyDpiX);
  QVERIFY(applyDpiY);
  QVERIFY(buttonBox);
  QVERIFY(applyDpiX->isEnabled());
  QVERIFY(applyDpiY->isEnabled());
  QVERIFY(!applyDpiX->isChecked());
  QVERIFY(!applyDpiY->isChecked());

  allPages->setChecked(true);
  applyLeft->setChecked(false);
  applyRight->setChecked(false);
  applyTop->setChecked(false);
  applyBottom->setChecked(false);
  applyDpiX->setChecked(true);
  applyDpiY->setChecked(false);
  buttonBox->button(QDialogButtonBox::Ok)->click();

  QTRY_VERIFY_WITH_TIMEOUT(!options->findChild<QDialog*>(), 10000);
  QTRY_COMPARE_WITH_TIMEOUT(pages->toPageSequence(PAGE_VIEW).pageAt(secondPage).metadata().dpi().horizontal(), 300,
                            10000);
  QCOMPARE(pages->toPageSequence(PAGE_VIEW).pageAt(secondPage).metadata().dpi().vertical(), 700);
  QCOMPARE(pages->toPageSequence(PAGE_VIEW).pageAt(firstPage).metadata().dpi(), Dpi(300, 300));

  auto* sourceXDpi = options->findChild<QLineEdit*>("sourceXDpi");
  auto* sourceYDpi = options->findChild<QLineEdit*>("sourceYDpi");
  QVERIFY(sourceXDpi);
  QVERIFY(sourceYDpi);
  sourceXDpi->setText(QStringLiteral("400"));
  sourceYDpi->setText(QStringLiteral("400"));
  QVERIFY(QMetaObject::invokeMethod(options, "sourceDpiEditingFinished", Qt::DirectConnection));
  QTRY_COMPARE_WITH_TIMEOUT(pages->toPageSequence(PAGE_VIEW).pageAt(firstPage).metadata().dpi().horizontal(), 400,
                            10000);
  QTRY_COMPARE_WITH_TIMEOUT(pages->toPageSequence(PAGE_VIEW).pageAt(firstPage).metadata().dpi().vertical(), 400, 10000);

  QTRY_VERIFY_WITH_TIMEOUT(window.m_workerThreadPool->isIdle(), 10000);
  options = window.m_optionsWidget.data();
  QVERIFY(options);
  applyButton = options->findChild<QPushButton*>("applySettingsBtn");
  QVERIFY(applyButton);
  applyButton->click();
  dialog = options->findChild<QDialog*>();
  QTRY_VERIFY(dialog);
  allPages = dialog->findChild<QRadioButton*>("allPagesRB");
  applyLeft = dialog->findChild<QCheckBox*>("applyLeftCB");
  applyRight = dialog->findChild<QCheckBox*>("applyRightCB");
  applyTop = dialog->findChild<QCheckBox*>("applyTopCB");
  applyBottom = dialog->findChild<QCheckBox*>("applyBottomCB");
  applyDpiX = dialog->findChild<QCheckBox*>("applyDpiXCB");
  applyDpiY = dialog->findChild<QCheckBox*>("applyDpiYCB");
  buttonBox = dialog->findChild<QDialogButtonBox*>("buttonBox");
  QVERIFY(allPages);
  QVERIFY(applyLeft);
  QVERIFY(applyRight);
  QVERIFY(applyTop);
  QVERIFY(applyBottom);
  QVERIFY(applyDpiX);
  QVERIFY(applyDpiY);
  QVERIFY(buttonBox);

  allPages->setChecked(true);
  applyLeft->setChecked(false);
  applyRight->setChecked(false);
  applyTop->setChecked(false);
  applyBottom->setChecked(false);
  applyDpiX->setChecked(false);
  applyDpiY->setChecked(true);
  buttonBox->button(QDialogButtonBox::Ok)->click();

  QTRY_COMPARE_WITH_TIMEOUT(pages->toPageSequence(PAGE_VIEW).pageAt(secondPage).metadata().dpi().horizontal(), 300,
                            10000);
  QCOMPARE(pages->toPageSequence(PAGE_VIEW).pageAt(secondPage).metadata().dpi().vertical(), 400);
}

void TestMainWindowAsync::filterDockWidthRestoresFromConfiguration() {
  SettingsStore settings;
  const QString key = QStringLiteral("mainWindow/dockState");
  ByteArraySettingRestore settingRestore(settings, key);
  settings.remove(key);

  int expectedWidth = 0;
  {
    MainWindow window;
    window.resize(1100, 700);
    window.setDockWidgetsVisible(true);
    window.show();
    QTRY_VERIFY(window.filterDockWidget->isVisible());

    window.resizeDocks({window.filterDockWidget}, {350}, Qt::Horizontal);
    QTRY_VERIFY_WITH_TIMEOUT(window.filterDockWidget->width() >= 340, 5000);
    expectedWidth = window.filterDockWidget->width();

    window.setDockWidgetsVisible(false);
    settings.setValue(key, window.saveState(1));
  }

  {
    MainWindow window;
    window.resize(1100, 700);
    window.setDockWidgetsVisible(true);
    window.show();
    QTRY_COMPARE_WITH_TIMEOUT(window.filterDockWidget->width(), expectedWidth, 5000);
  }
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
