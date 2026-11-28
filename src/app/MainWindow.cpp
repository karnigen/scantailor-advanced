// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#include "MainWindow.h"

#include <core/ApplicationSettings.h>
#include <core/IconProvider.h>
#include <core/PathUtils.h>
#include <core/SettingsStore.h>

#include <QActionGroup>
#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QCoreApplication>
#include <QDebug>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QEventLoop>
#include <QFileDialog>
#include <QFileInfo>
#include <QFileSystemModel>
#include <QFont>
#include <QFormLayout>
#include <QFrame>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QImageReader>
#include <QKeySequence>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMenu>
#include <QMessageBox>
#include <QPainter>
#include <QPalette>
#include <QProcess>
#include <QProgressDialog>
#include <QPushButton>
#include <QResource>
#include <QScrollBar>
#include <QSet>
#include <QSortFilterProxyModel>
#include <QSpinBox>
#include <QStackedLayout>
#include <QStandardPaths>
#include <QStringList>
#include <QStyle>
#include <QTemporaryFile>
#include <QTextBrowser>
#include <QThread>
#include <QTimer>
#include <QVBoxLayout>
#include <QtWidgets/QInputDialog>
#include <algorithm>
#include <atomic>
#include <cmath>
#include <exception>
#include <limits>
#include <memory>

#include "AbstractRelinker.h"
#include "Application.h"
#include "AutoRemovingFile.h"
#include "BasicImageView.h"
#include "CompletionSound.h"
#include "ConfigurationDialog.h"
#include "ContentBoxPropagator.h"
#include "DebugImageView.h"
#include "DebugImages.h"
#include "DefaultParamsDialog.h"
#include "ErrorWidget.h"
#include "FilterOptionsWidget.h"
#include "FixDpiDialog.h"
#include "ImageInfo.h"
#include "ImageMetadataLoader.h"
#include "ImageViewBase.h"
#include "LoadFileTask.h"
#include "LoadFilesStatusDialog.h"
#include "NewOpenProjectPanel.h"
#include "OutOfMemoryDialog.h"
#include "OutOfMemoryHandler.h"
#include "PageOrientationPropagator.h"
#include "PageSelectionAccessor.h"
#include "PageSequence.h"
#include "ProcessedPagesExporter.h"
#include "ProcessingIndicationWidget.h"
#include "ProcessingTaskQueue.h"
#include "ProjectBackup.h"
#include "ProjectCreationContext.h"
#include "ProjectMigration.h"
#include "ProjectOpeningContext.h"
#include "ProjectPages.h"
#include "ProjectReader.h"
#include "ProjectWriter.h"
#include "RecentProjects.h"
#include "RelinkingDialog.h"
#include "ScopedIncDec.h"
#include "SettingsDialog.h"
#include "SkinnedButton.h"
#include "SmartFilenameOrdering.h"
#include "StageSequence.h"
#include "SystemLoadWidget.h"
#include "TabbedDebugImages.h"
#include "ThumbnailFactory.h"
#include "ThumbnailViewMode.h"
#include "TiffWriter.h"
#include "UnitsProvider.h"
#include "Utils.h"
#include "WorkerThreadPool.h"
#include "config.h"
#include "filters/deskew/CacheDrivenTask.h"
#include "filters/deskew/Task.h"
#include "filters/fix_orientation/CacheDrivenTask.h"
#include "filters/fix_orientation/Task.h"
#include "filters/output/CacheDrivenTask.h"
#include "filters/output/TabbedImageView.h"
#include "filters/output/Task.h"
#include "filters/page_layout/CacheDrivenTask.h"
#include "filters/page_layout/Task.h"
#include "filters/page_split/CacheDrivenTask.h"
#include "filters/page_split/Task.h"
#include "filters/select_content/CacheDrivenTask.h"
#include "filters/select_content/Task.h"
#include "ui_AboutDialog.h"
#include "ui_BatchProcessingLowerPanel.h"
#include "ui_RemovePagesDialog.h"
#include "version.h"

using namespace core;

namespace {

QString findPdfHelperScript(const QString& fileName) {
  QStringList applicationDirs{QCoreApplication::applicationDirPath()};
  const QString executablePath = QFileInfo(QCoreApplication::applicationFilePath()).canonicalFilePath();
  if (!executablePath.isEmpty()) {
    const QString resolvedDir = QFileInfo(executablePath).absolutePath();
    if (!applicationDirs.contains(resolvedDir)) {
      applicationDirs.push_back(resolvedDir);
    }
  }

  QStringList candidates;
  for (const QString& applicationDirPath : applicationDirs) {
    const QDir applicationDir(applicationDirPath);
    candidates << applicationDir.filePath(QStringLiteral("share/scantailor-advanced/pdf-tools/%1").arg(fileName))
               << applicationDir.filePath(QStringLiteral("../share/scantailor-advanced/pdf-tools/%1").arg(fileName))
               << applicationDir.filePath(QStringLiteral("../Resources/pdf-tools/%1").arg(fileName));
  }
  for (const QString& candidate : candidates) {
    const QFileInfo fileInfo(candidate);
    if (fileInfo.isFile()) {
      return fileInfo.absoluteFilePath();
    }
  }
  return {};
}

QString menuTitleWithoutMnemonic(const QString& title, QChar* mnemonic = nullptr) {
  QString visibleTitle;
  for (int i = 0; i < title.size(); ++i) {
    if (title.at(i) != '&' || i + 1 == title.size()) {
      visibleTitle += title.at(i);
      continue;
    }

    if (title.at(i + 1) == '&') {
      visibleTitle += '&';
      ++i;
      continue;
    }

    if (mnemonic && mnemonic->isNull()) {
      *mnemonic = title.at(i + 1);
    }
    visibleTitle += title.at(++i);
  }
  return visibleTitle;
}

bool runExporterWithProgress(
    QProgressDialog& progress,
    const std::function<bool(const ProcessedPagesExporter::ProgressCallback&, QString*)>& exportOperation,
    QString* errorMessage) {
  std::atomic_bool cancelRequested = false;
  QObject::connect(&progress, &QProgressDialog::canceled, &progress,
                   [&cancelRequested]() { cancelRequested.store(true); });

  bool success = false;
  QString workerError;
  QEventLoop eventLoop;
  QThread* worker = QThread::create([&]() {
    const ProcessedPagesExporter::ProgressCallback progressCallback
        = [&progress, &cancelRequested](const int completed, const int total) {
            return QMetaObject::invokeMethod(
                       &progress,
                       [&progress, completed, total]() {
                         progress.setMaximum(total);
                         progress.setValue(completed);
                       },
                       Qt::BlockingQueuedConnection)
                   && !cancelRequested.load();
          };
    success = exportOperation(progressCallback, &workerError);
  });
  QObject::connect(worker, &QThread::finished, &eventLoop, &QEventLoop::quit);

  progress.setAutoClose(false);
  progress.setAutoReset(false);
  progress.show();
  worker->start();
  eventLoop.exec();
  worker->wait();
  delete worker;
  progress.close();

  if (errorMessage) {
    *errorMessage = workerError;
  }
  return success;
}

}  // namespace

class MainWindow::PageSelectionProviderImpl : public PageSelectionProvider {
 public:
  explicit PageSelectionProviderImpl(MainWindow* wnd) : m_wnd(wnd) {}

  PageSequence allPages() const override { return m_wnd ? m_wnd->allPages() : PageSequence(); }

  std::set<PageId> selectedPages() const override { return m_wnd ? m_wnd->selectedPages() : std::set<PageId>(); }

  std::vector<PageRange> selectedRanges() const override {
    return m_wnd ? m_wnd->selectedRanges() : std::vector<PageRange>();
  }

 private:
  QPointer<MainWindow> m_wnd;
};


MainWindow::MainWindow(std::filesystem::path initialDirectory)
    : m_pages(std::make_shared<ProjectPages>()),
      m_stages(std::make_shared<StageSequence>(m_pages, newPageSelectionAccessor())),
      m_workerThreadPool(std::make_unique<WorkerThreadPool>()),
      m_interactiveQueue(std::make_unique<ProcessingTaskQueue>()),
      m_outOfMemoryDialog(std::make_unique<OutOfMemoryDialog>()),
      m_curFilter(0),
      m_savedMainAreaViewState(),
      m_ignoreSelectionChanges(0),
      m_ignorePageOrderingChanges(0),
      m_debug(false),
      m_closing(false) {
  m_initialDirectory = std::move(initialDirectory);
  ApplicationSettings& settings = ApplicationSettings::getInstance();

  m_maxLogicalThumbSize = settings.getMaxLogicalThumbnailSize();
  const ThumbnailSequence::ViewMode viewMode = settings.isSingleColumnThumbnailDisplayEnabled()
                                                   ? ThumbnailSequence::SINGLE_COLUMN
                                                   : ThumbnailSequence::MULTI_COLUMN;
  m_thumbSequence = std::make_unique<ThumbnailSequence>(m_maxLogicalThumbSize, viewMode);

  m_autoSaveTimer.setSingleShot(true);
  connect(&m_autoSaveTimer, SIGNAL(timeout()), SLOT(autoSaveProject()));

  setupUi(this);
  setupIcons();
  focusButton->setChecked(SettingsStore().boolValue("mainWindow/keepCurrentPageInView", true));

  applyMenuMnemonics();

  sortOptions->setVisible(false);

  createBatchProcessingWidget();
  m_processingIndicationWidget = std::make_unique<ProcessingIndicationWidget>();

  filterList->setStages(m_stages);
  filterList->selectRow(0);

  setupThumbView();  // Expects m_thumbSequence to be initialized.
  m_tabbedDebugImages = std::make_unique<TabbedDebugImages>();

  m_debug = actionDebug->isChecked();

  m_imageFrameLayout = new QStackedLayout(imageViewFrame);
  m_imageFrameLayout->setStackingMode(QStackedLayout::StackAll);

  m_optionsFrameLayout = new QStackedLayout(filterOptions);

  m_statusBarPanel = new StatusBarPanel;
  QMainWindow::statusBar()->addPermanentWidget(m_statusBarPanel);
  connect(m_thumbSequence.get(), &ThumbnailSequence::newSelectionLeader, [this](const PageInfo& pageInfo) {
    PageSequence pageSequence = m_thumbSequence->toPageSequence();
    if (pageSequence.numPages() > 0) {
      m_statusBarPanel->updatePage(pageSequence.pageNo(pageInfo.id()) + 1, pageSequence.numPages(), pageInfo.id());
    } else {
      m_statusBarPanel->clear();
    }
  });

  m_unitsMenuActionGroup = new QActionGroup(this);
  for (QAction* action : menuUnits->actions()) {
    m_unitsMenuActionGroup->addAction(action);
  }
  switch (unitsFromString(settings.getUnits())) {
    case PIXELS:
      actionPixels->setChecked(true);
      break;
    case MILLIMETRES:
      actionMilimeters->setChecked(true);
      break;
    case CENTIMETRES:
      actionCentimetres->setChecked(true);
      break;
    case INCHES:
      actionInches->setChecked(true);
      break;
  }
  connect(actionPixels, &QAction::toggled, [this, &settings](bool checked) {
    if (checked) {
      UnitsProvider::getInstance().setUnits(PIXELS);
      settings.setUnits(unitsToString(PIXELS));
    }
  });
  connect(actionMilimeters, &QAction::toggled, [this, &settings](bool checked) {
    if (checked) {
      UnitsProvider::getInstance().setUnits(MILLIMETRES);
      settings.setUnits(unitsToString(MILLIMETRES));
    }
  });
  connect(actionCentimetres, &QAction::toggled, [this, &settings](bool checked) {
    if (checked) {
      UnitsProvider::getInstance().setUnits(CENTIMETRES);
      settings.setUnits(unitsToString(CENTIMETRES));
    }
  });
  connect(actionInches, &QAction::toggled, [this, &settings](bool checked) {
    if (checked) {
      UnitsProvider::getInstance().setUnits(INCHES);
      settings.setUnits(unitsToString(INCHES));
    }
  });

  setSingleColumnViewButtonState(*thumbColumnViewBtn, settings.isSingleColumnThumbnailDisplayEnabled());
  deviationHighlightingBtn->setChecked(settings.isHighlightDeviationEnabled());

  addAction(actionFirstPage);
  addAction(actionLastPage);
  addAction(actionNextPage);
  addAction(actionPrevPage);
  addAction(actionPrevPageQ);
  addAction(actionNextPageW);
  addAction(actionPrevTenPagesA);
  addAction(actionNextTenPagesS);
  addAction(actionNextSelectedPage);
  addAction(actionPrevSelectedPage);
  addAction(actionNextSelectedPageW);
  addAction(actionPrevSelectedPageQ);
  addAction(actionGotoPage);
  addAction(actionMagnifyThumbnails);
  addAction(actionDiminishThumbnails);
  addAction(actionReloadPage);

  addAction(actionSwitchFilter1);
  addAction(actionSwitchFilter2);
  addAction(actionSwitchFilter3);
  addAction(actionSwitchFilter4);
  addAction(actionSwitchFilter5);
  addAction(actionSwitchFilter6);
  // Should be enough to save a project.
  OutOfMemoryHandler::instance().allocateEmergencyMemory(3 * 1024 * 1024);

  connect(actionFirstPage, SIGNAL(triggered(bool)), SLOT(goFirstPage()));
  connect(actionLastPage, SIGNAL(triggered(bool)), SLOT(goLastPage()));
  connect(actionPrevPage, SIGNAL(triggered(bool)), SLOT(goPrevPage()));
  connect(actionNextPage, SIGNAL(triggered(bool)), SLOT(goNextPage()));
  connect(actionPrevPageQ, SIGNAL(triggered(bool)), this, SLOT(goPrevPage()));
  connect(actionNextPageW, SIGNAL(triggered(bool)), this, SLOT(goNextPage()));
  connect(actionPrevTenPagesA, SIGNAL(triggered(bool)), this, SLOT(goPrevTenPages()));
  connect(actionNextTenPagesS, SIGNAL(triggered(bool)), this, SLOT(goNextTenPages()));
  connect(actionPrevSelectedPage, SIGNAL(triggered(bool)), SLOT(goPrevSelectedPage()));
  connect(actionNextSelectedPage, SIGNAL(triggered(bool)), SLOT(goNextSelectedPage()));
  connect(actionPrevSelectedPageQ, SIGNAL(triggered(bool)), this, SLOT(goPrevSelectedPage()));
  connect(actionNextSelectedPageW, SIGNAL(triggered(bool)), this, SLOT(goNextSelectedPage()));
  connect(actionGotoPage, SIGNAL(triggered(bool)), this, SLOT(execGotoPageDialog()));
  connect(actionAbout, SIGNAL(triggered(bool)), this, SLOT(showAboutDialog()));
  connect(actionKeyboardShortcuts, &QAction::triggered, this, &MainWindow::showKeyboardShortcuts);
  connect(actionSupportedImageFormats, &QAction::triggered, this, &MainWindow::showSupportedImageFormats);
  connect(actionExportProcessedImages, &QAction::triggered, this, &MainWindow::exportProcessedImages);
  connect(actionExportPagesToPdf, &QAction::triggered, this, &MainWindow::exportPagesToPdf);
  connect(actionExtractPdfImages, &QAction::triggered, this, &MainWindow::extractPdfImages);
  connect(actionRenderPdfPages, &QAction::triggered, this, &MainWindow::renderPdfPages);
  connect(actionCommandLineArguments, &QAction::triggered, this, &MainWindow::showCommandLineArguments);
  connect(actionConfiguration, &QAction::triggered, this, &MainWindow::showConfigurationDialog);
  connect(&OutOfMemoryHandler::instance(), SIGNAL(outOfMemory()), SLOT(handleOutOfMemorySituation()));
  connect(prevPageBtn, &QToolButton::clicked, this, [this]() {
    if (filterSelectedBtn->isChecked()) {
      goPrevSelectedPage();
    } else {
      goPrevPage();
    }
  });
  connect(nextPageBtn, &QToolButton::clicked, this, [this]() {
    if (filterSelectedBtn->isChecked()) {
      goNextSelectedPage();
    } else {
      goNextPage();
    }
  });
  connect(gotoPageBtn, SIGNAL(clicked()), this, SLOT(execGotoPageDialog()));

  auto magnifyThumbnails = [this]() { scaleThumbnails(1); };
  auto diminishThumbnails = [this]() { scaleThumbnails(-1); };
  connect(magnifyThumbnailsBtn, &QPushButton::clicked, magnifyThumbnails);
  connect(diminishThumbnailsBtn, &QPushButton::clicked, diminishThumbnails);
  connect(actionMagnifyThumbnails, &QAction::triggered, magnifyThumbnails);
  connect(actionDiminishThumbnails, &QAction::triggered, diminishThumbnails);
  connect(actionReloadPage, SIGNAL(triggered(bool)), SLOT(reloadCurrentPage()));

  connect(actionSwitchFilter1, SIGNAL(triggered(bool)), SLOT(switchFilter1()));
  connect(actionSwitchFilter2, SIGNAL(triggered(bool)), SLOT(switchFilter2()));
  connect(actionSwitchFilter3, SIGNAL(triggered(bool)), SLOT(switchFilter3()));
  connect(actionSwitchFilter4, SIGNAL(triggered(bool)), SLOT(switchFilter4()));
  connect(actionSwitchFilter5, SIGNAL(triggered(bool)), SLOT(switchFilter5()));
  connect(actionSwitchFilter6, SIGNAL(triggered(bool)), SLOT(switchFilter6()));

  connect(filterList->selectionModel(), SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)), this,
          SLOT(filterSelectionChanged(const QItemSelection&)));
  connect(filterList, SIGNAL(launchBatchProcessing()), this, SLOT(startBatchProcessing()));

  connect(m_workerThreadPool.get(), &WorkerThreadPool::taskResult, this, &MainWindow::filterResult);
  connect(m_workerThreadPool.get(), &WorkerThreadPool::idle, this, &MainWindow::finishWaitingForCancelledTasks);

  connect(m_thumbSequence.get(),
          SIGNAL(newSelectionLeader(const PageInfo&, const QRectF&, ThumbnailSequence::SelectionFlags)), this,
          SLOT(currentPageChanged(const PageInfo&, const QRectF&, ThumbnailSequence::SelectionFlags)));
  connect(m_thumbSequence.get(), SIGNAL(pageContextMenuRequested(const PageInfo&, const QPoint&, bool)), this,
          SLOT(pageContextMenuRequested(const PageInfo&, const QPoint&, bool)));
  connect(m_thumbSequence.get(), SIGNAL(pastLastPageContextMenuRequested(const QPoint&)),
          SLOT(pastLastPageContextMenuRequested(const QPoint&)));
  connect(selectionModeBtn, SIGNAL(clicked(bool)), m_thumbSequence.get(), SLOT(setSelectionModeEnabled(bool)));

  connect(focusButton, &QToolButton::clicked, this, [this](bool checked) {
    SettingsStore().setValue("mainWindow/keepCurrentPageInView", checked);
    thumbViewFocusToggled(checked);
  });
  connect(sortOptions, SIGNAL(currentIndexChanged(int)), this, SLOT(pageOrderingChanged(int)));
  connect(thumbColumnViewBtn, &QToolButton::clicked, this, [this, &settings](bool checked) {
    settings.setSingleColumnThumbnailDisplayEnabled(checked);
    updateThumbnailViewMode();
    m_thumbSequence->updateSceneItemsPos();
  });
  connect(sortingOrderBtn, &QToolButton::clicked, this,
          [this](bool) { pageOrderingChanged(m_stages->filterAt(m_curFilter)->selectedPageOrder()); });
  connect(pauseSortingBtn, &QToolButton::toggled, this, [this](bool) {
    if (!isProjectLoaded()) {
      return;
    }
    resetThumbSequence(currentPageOrderProvider(), ThumbnailSequence::KEEP_SELECTION);
  });
  connect(deviationHighlightingBtn, &QToolButton::clicked, this, [this, &settings](bool checked) {
    settings.setHighlightDeviationEnabled(checked);
    m_thumbSequence->invalidateAllThumbnails();
  });

  connect(actionFixDpi, SIGNAL(triggered(bool)), SLOT(fixDpiDialogRequested()));
  connect(actionRelinking, SIGNAL(triggered(bool)), SLOT(showRelinkingDialog()));
#ifdef ENABLE_DEBUG_FEATURES
  connect(actionDebug, SIGNAL(toggled(bool)), SLOT(debugToggled(bool)));
#else
  actionDebug->setVisible(false);
#endif

  connect(actionSettings, SIGNAL(triggered(bool)), this, SLOT(openSettingsDialog()));
  connect(actionDefaults, SIGNAL(triggered(bool)), this, SLOT(openDefaultParamsDialog()));

  connect(actionNewProject, SIGNAL(triggered(bool)), this, SLOT(newProject()));
  connect(actionOpenProject, SIGNAL(triggered(bool)), this, SLOT(openProject()));
  connect(actionReloadProject, &QAction::triggered, this, &MainWindow::reloadProject);
  connect(actionSaveProject, SIGNAL(triggered(bool)), this, SLOT(saveProjectTriggered()));
  connect(actionSaveProjectAs, SIGNAL(triggered(bool)), this, SLOT(saveProjectAsTriggered()));
  connect(actionCloseProject, SIGNAL(triggered(bool)), this, SLOT(closeProject()));
  connect(actionQuit, SIGNAL(triggered(bool)), this, SLOT(close()));

  updateProjectActions();
  updateWindowTitle();
  updateMainArea();

  SettingsStore qSettings;
  const QByteArray dockState = qSettings.byteArrayValue("mainWindow/dockState");
  if (!dockState.isEmpty()) {
    restoreState(dockState, 1);
  }
  if (!qSettings.boolValue("mainWindow/maximized", true)) {
    if (!restoreGeometry(qSettings.byteArrayValue("mainWindow/nonMaximizedGeometry"))) {
      resize(1014, 689);  // A sensible value.
    }
  }

  m_maxLogicalThumbSizeUpdater.setSingleShot(true);
  connect(&m_maxLogicalThumbSizeUpdater, &QTimer::timeout, this, &MainWindow::updateMaxLogicalThumbSize);

  m_sceneItemsPosUpdater.setSingleShot(true);
  connect(&m_sceneItemsPosUpdater, &QTimer::timeout, m_thumbSequence.get(), &ThumbnailSequence::updateSceneItemsPos);

  m_projectModifiedTimer.setSingleShot(true);
  connect(&m_projectModifiedTimer, &QTimer::timeout, this, &MainWindow::updateProjectModifiedState);
  qApp->installEventFilter(this);
}

MainWindow::~MainWindow() {
  if (m_pdfOptimizationThread) {
    m_pdfOptimizationThread->wait();
  }
  qApp->removeEventFilter(this);
  m_interactiveQueue->cancelAndClear();
  if (m_batchQueue) {
    m_batchQueue->cancelAndClear();
  }
  m_workerThreadPool->shutdown();

  removeWidgetsFromLayout(m_imageFrameLayout);
  removeWidgetsFromLayout(m_optionsFrameLayout);
  m_tabbedDebugImages->clear();
}

PageSequence MainWindow::allPages() const {
  return m_thumbSequence->toPageSequence();
}

std::set<PageId> MainWindow::selectedPages() const {
  return m_thumbSequence->selectedItems();
}

std::vector<PageRange> MainWindow::selectedRanges() const {
  return m_thumbSequence->selectedRanges();
}

void MainWindow::switchToNewProject(const std::shared_ptr<ProjectPages>& pages,
                                    const std::filesystem::path& outDir,
                                    const std::filesystem::path& projectFilePath,
                                    const ProjectReader* projectReader) {
  stopBatchProcessing(CLEAR_MAIN_AREA);
  m_interactiveQueue->cancelAndClear();

  if (!outDir.empty()) {
    Utils::maybeCreateCacheDir(outDir);
  }
  m_pages = pages;
  m_projectFile = projectFilePath;

  if (projectReader) {
    m_selectedPage = projectReader->selectedPage();
  }

  std::shared_ptr<FileNameDisambiguator> disambiguator;
  if (projectReader) {
    disambiguator = projectReader->namingDisambiguator();
  } else {
    disambiguator.reset(new FileNameDisambiguator);
  }

  m_outFileNameGen = OutputFileNameGenerator(disambiguator, outDir, pages->layoutDirection(), pages);
  // These two need to go in this order.
  updateDisambiguationRecords(pages->toPageSequence(IMAGE_VIEW));

  // Recreate the stages and load their state.
  m_stages = std::make_shared<StageSequence>(pages, newPageSelectionAccessor());
  if (projectReader) {
    projectReader->readFilterSettings(m_stages->filters());
  }

  // Connect the filter list model to the view and select
  // the first item.
  {
    ScopedIncDec<int> guard(m_ignoreSelectionChanges);
    filterList->setStages(m_stages);
    filterList->selectRow(0);
    m_curFilter = 0;
    // Setting a data model also implicitly sets a new
    // selection model, so we have to reconnect to it.
    connect(filterList->selectionModel(), SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)), this,
            SLOT(filterSelectionChanged(const QItemSelection&)));
  }

  updateSortOptions();

  m_contentBoxPropagator = std::make_unique<ContentBoxPropagator>(
      m_stages->pageLayoutFilter(), createCompositeCacheDrivenTask(m_stages->selectContentFilterIdx()));

  m_pageOrientationPropagator = std::make_unique<PageOrientationPropagator>(
      m_stages->pageSplitFilter(), createCompositeCacheDrivenTask(m_stages->fixOrientationFilterIdx()));

  // Thumbnails are stored relative to the output directory,
  // so recreate the thumbnail cache.
  if (outDir.empty()) {
    m_thumbnailCache.reset();
  } else {
    m_thumbnailCache = Utils::createThumbnailCache(m_outFileNameGen.outDir());
  }
  resetThumbSequence(currentPageOrderProvider());

  removeFilterOptionsWidget();
  updateProjectActions();
  m_projectModified
      = isProjectLoaded() && (m_projectFile.empty() || !QFileInfo::exists(core::toQString(m_projectFile)));
  updateWindowTitle();
  if (isProjectLoaded() && !m_projectModified) {
    updateProjectModifiedState();
  }
  updateMainArea();

  connect(m_pages.get(), &ProjectPages::modified, this, [this]() {
    if (isProjectLoaded()) {
      m_projectModifiedTimer.start(300);
    }
  });
  connect(m_pages.get(), &ProjectPages::pageOrderChanged, this, [this]() {
    resetThumbSequence(currentPageOrderProvider(), ThumbnailSequence::KEEP_SELECTION);
    m_thumbSequence->invalidateAllThumbnails();
    updateMainArea();
  });
}  // MainWindow::switchToNewProject

void MainWindow::showNewOpenProjectPanel() {
  auto outerWidget = std::make_unique<QWidget>();
  auto* layout = new QGridLayout(outerWidget.get());
  outerWidget->setLayout(layout);

  auto* nop = new NewOpenProjectPanel(outerWidget.get());
  // We use asynchronous connections because otherwise we
  // would be deleting a widget from its event handler, which
  // Qt doesn't like.
  connect(nop, SIGNAL(newProject()), this, SLOT(newProject()), Qt::QueuedConnection);
  connect(nop, SIGNAL(openProject()), this, SLOT(openProject()), Qt::QueuedConnection);
  connect(nop, SIGNAL(openRecentProject(const QString&)), this, SLOT(openProject(const QString&)),
          Qt::QueuedConnection);

  layout->addWidget(nop, 1, 1);
  layout->setColumnStretch(0, 1);
  layout->setColumnStretch(2, 1);
  layout->setRowStretch(0, 1);
  layout->setRowStretch(2, 1);
  setImageWidget(outerWidget.release(), TRANSFER_OWNERSHIP);

  filterList->setBatchProcessingPossible(false);
}  // MainWindow::showNewOpenProjectPanel

void MainWindow::createBatchProcessingWidget() {
  m_batchProcessingWidget = std::make_unique<QWidget>();
  auto* layout = new QGridLayout(m_batchProcessingWidget.get());
  m_batchProcessingWidget->setLayout(layout);

  const auto& iconProvider = IconProvider::getInstance();
  auto* stopBtn = new SkinnedButton(iconProvider.getIcon("stop"), iconProvider.getIcon("stop-hovered"),
                                    iconProvider.getIcon("stop-pressed"), m_batchProcessingWidget.get());
  stopBtn->setIconSize({128, 128});
  stopBtn->setStatusTip(tr("Stop batch processing"));
  stopBtn->setToolTip(tr("Stop the current batch-processing run."));

  class LowerPanel : public QWidget {
   public:
    explicit LowerPanel(QWidget* parent = 0) : QWidget(parent) { ui.setupUi(this); }

    Ui::BatchProcessingLowerPanel ui;
  };


  auto* lowerPanel = new LowerPanel(m_batchProcessingWidget.get());
  m_checkBeepWhenFinished = [lowerPanel]() { return lowerPanel->ui.beepWhenFinished->isChecked(); };

  int row = 0;  // Row 0 is reserved.
  layout->addWidget(stopBtn, ++row, 1, Qt::AlignCenter);
  layout->addWidget(lowerPanel, ++row, 0, 1, 3, Qt::AlignHCenter | Qt::AlignTop);
  layout->setColumnStretch(0, 1);
  layout->setColumnStretch(2, 1);
  layout->setRowStretch(0, 1);
  layout->setRowStretch(row, 1);

  connect(stopBtn, SIGNAL(clicked()), SLOT(stopBatchProcessing()));
}  // MainWindow::createBatchProcessingWidget

void MainWindow::updateThumbViewMinWidth() {
  const int sb = thumbView->style()->pixelMetric(QStyle::PM_ScrollBarExtent);
  int innerWidth = thumbView->maximumViewportSize().width() - sb;
  if (thumbView->style()->styleHint(QStyle::SH_ScrollView_FrameOnlyAroundContents, nullptr, thumbView)) {
    innerWidth -= thumbView->frameWidth() * 2;
  }
  const int deltaX = thumbView->size().width() - innerWidth;
  thumbView->setMinimumWidth((int) std::ceil(m_maxLogicalThumbSize.width() + deltaX));
}

void MainWindow::setupThumbView() {
  updateThumbViewMinWidth();
  thumbView->setRenderHint(QPainter::SmoothPixmapTransform, true);
  m_thumbSequence->attachView(thumbView);
  thumbView->installEventFilter(this);
  thumbView->viewport()->installEventFilter(this);
}

bool MainWindow::eventFilter(QObject* obj, QEvent* ev) {
  if (isProjectLoaded() && !m_closing
      && (ev->type() == QEvent::MouseButtonRelease || ev->type() == QEvent::KeyRelease)) {
    m_projectModifiedTimer.start(300);
  }

  if ((obj == thumbView) && (ev->type() == QEvent::Resize)) {
    if (!m_sceneItemsPosUpdater.isActive()) {
      m_sceneItemsPosUpdater.start(150);
    }
  }

  const bool isThumbnailView = (obj == thumbView) || (obj == thumbView->viewport());
  if (isThumbnailView && ev->type() == QEvent::Wheel) {
    auto* wheelEvent = static_cast<QWheelEvent*>(ev);
    const Qt::KeyboardModifiers modifiers = wheelEvent->modifiers();
    const bool singlePageNavigation = modifiers == Qt::ControlModifier;
    const bool fastPageNavigation = modifiers == Qt::ShiftModifier;
    if (singlePageNavigation || fastPageNavigation) {
      const QPoint pixelDelta = wheelEvent->pixelDelta();
      const QPoint angleDelta = wheelEvent->angleDelta();
      const int wheelDelta = !pixelDelta.isNull() ? (pixelDelta.y() != 0 ? pixelDelta.y() : pixelDelta.x())
                                                  : (angleDelta.y() != 0 ? angleDelta.y() : angleDelta.x());
      int& wheelDeltaAccumulator
          = singlePageNavigation ? m_thumbnailNavigationWheelDelta : m_thumbnailFastNavigationWheelDelta;
      wheelDeltaAccumulator += wheelDelta;
      const int pageSteps = wheelDeltaAccumulator / 120;
      wheelDeltaAccumulator %= 120;
      if (pageSteps != 0) {
        navigatePages(pageSteps, fastPageNavigation ? 10 : 1);
      }
      wheelEvent->accept();
      return true;
    }

    if (wheelEvent->modifiers() == Qt::AltModifier) {
      const QPoint pixelDelta = wheelEvent->pixelDelta();
      const QPoint angleDelta = wheelEvent->angleDelta();
      const double scaleSteps
          = !pixelDelta.isNull() ? (pixelDelta.x() + pixelDelta.y()) / 40.0 : (angleDelta.x() + angleDelta.y()) / 120.0;
      if (scaleSteps != 0.0) {
        scaleThumbnails(scaleSteps, isThumbnailView);
      }
      wheelEvent->accept();
      return true;
    }
  }
  return false;
}

void MainWindow::closeEvent(QCloseEvent* const event) {
  if (m_closing) {
    event->accept();
  } else {
    event->ignore();
    startTimer(0);
  }
}

void MainWindow::timerEvent(QTimerEvent* const event) {
  // We only use the timer event for delayed closing of the window.
  killTimer(event->timerId());

  if (closeProjectInteractive()) {
    m_closing = true;
    SettingsStore settings;
    settings.setValue("mainWindow/maximized", isMaximized());
    settings.setValue("mainWindow/dockState", saveState(1));
    if (!isMaximized()) {
      settings.setValue("mainWindow/nonMaximizedGeometry", saveGeometry());
    }
    close();
  }
}

MainWindow::SavePromptResult MainWindow::promptProjectSave() {
  QMessageBox msgBox(QMessageBox::Question, tr("Save Project"), tr("Save the project?"),
                     QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel, this);
  msgBox.setDefaultButton(QMessageBox::Save);
  auto* saveButton = msgBox.button(QMessageBox::Save);
  saveButton->setShortcut(QKeySequence(Qt::AltModifier | Qt::Key_S));
  saveButton->setToolTip(tr("Save the project. Shortcut: Alt+S."));
  auto* discardButton = msgBox.button(QMessageBox::Discard);
  discardButton->setShortcut(QKeySequence(Qt::AltModifier | Qt::Key_D));
  discardButton->setToolTip(tr("Discard project changes. Shortcut: Alt+D."));
  auto makeButtonTextFit = [](QAbstractButton* button) {
    const int iconWidth = button->icon().isNull() ? 0 : button->iconSize().width();
    const int requiredWidth = button->fontMetrics().horizontalAdvance(button->text()) + iconWidth + 48;
    button->setMinimumWidth(qMax(button->sizeHint().width(), requiredWidth));
  };
  makeButtonTextFit(saveButton);
  makeButtonTextFit(discardButton);
  makeButtonTextFit(msgBox.button(QMessageBox::Cancel));

  switch (msgBox.exec()) {
    case QMessageBox::Save:
      return SAVE;
    case QMessageBox::Discard:
      return DONT_SAVE;
    default:
      return CANCEL;
  }
}

bool MainWindow::compareFiles(const QString& fpath1, const QString& fpath2) {
  QFile file1(fpath1);
  QFile file2(fpath2);

  if (!file1.open(QIODevice::ReadOnly)) {
    return false;
  }
  if (!file2.open(QIODevice::ReadOnly)) {
    return false;
  }

  if (!file1.isSequential() && !file2.isSequential()) {
    if (file1.size() != file2.size()) {
      return false;
    }
  }

  const int chunkSize = 4096;
  while (true) {
    const QByteArray chunk1(file1.read(chunkSize));
    const QByteArray chunk2(file2.read(chunkSize));
    if (chunk1.size() != chunk2.size()) {
      return false;
    }
    if (chunk1 != chunk2) {
      return false;
    }
    if (chunk1.isEmpty()) {
      return true;
    }
  }
}

std::shared_ptr<const PageOrderProvider> MainWindow::currentPageOrderProvider() const {
  if (pauseSortingBtn->isChecked()) {
    return nullptr;
  }

  const int idx = sortOptions->currentIndex();
  if (idx < 0) {
    return nullptr;
  }

  const std::shared_ptr<AbstractFilter> filter(m_stages->filterAt(m_curFilter));
  std::shared_ptr<const PageOrderProvider> currentOrderProvider = filter->pageOrderOptions()[idx].provider();
  return (currentOrderProvider && sortingOrderBtn->isChecked()) ? currentOrderProvider->reversed()
                                                                : currentOrderProvider;
}

void MainWindow::updateSortOptions() {
  const ScopedIncDec<int> guard(m_ignorePageOrderingChanges);

  const std::shared_ptr<AbstractFilter> filter(m_stages->filterAt(m_curFilter));

  sortOptions->clear();

  for (const PageOrderOption& opt : filter->pageOrderOptions()) {
    sortOptions->addItem(opt.name());
  }

  sortOptions->setVisible(sortOptions->count() > 0);

  pauseSortingBtn->setEnabled(sortOptions->count() > 0);
  if (sortOptions->count() == 0) {
    pauseSortingBtn->setChecked(false);
  }

  if (sortOptions->count() > 0) {
    sortOptions->setCurrentIndex(filter->selectedPageOrder());
  }
}

void MainWindow::resetThumbSequence(const std::shared_ptr<const PageOrderProvider>& pageOrderProvider,
                                    const ThumbnailSequence::SelectionAction selectionAction) {
  if (m_thumbnailCache) {
    const std::shared_ptr<CompositeCacheDrivenTask> task(createCompositeCacheDrivenTask(m_curFilter));

    m_thumbSequence->setThumbnailFactory(
        std::make_shared<ThumbnailFactory>(m_thumbnailCache, m_maxLogicalThumbSize, task));
  }

  m_thumbSequence->reset(currentPageSequence(), selectionAction, pageOrderProvider);

  if (!m_thumbnailCache) {
    // Empty project.
    assert(m_pages->numImages() == 0);
    m_thumbSequence->setThumbnailFactory(nullptr);
  }

  if (selectionAction != ThumbnailSequence::KEEP_SELECTION) {
    const PageId page(m_selectedPage.get(getCurrentView()));
    if (m_thumbSequence->setSelection(page)) {
      // OK
    } else if (m_thumbSequence->setSelection(PageId(page.imageId(), PageId::LEFT_PAGE))) {
      // OK
    } else if (m_thumbSequence->setSelection(PageId(page.imageId(), PageId::RIGHT_PAGE))) {
      // OK
    } else if (m_thumbSequence->setSelection(PageId(page.imageId(), PageId::SINGLE_PAGE))) {
      // OK
    } else {
      // Last resort.
      m_thumbSequence->setSelection(m_thumbSequence->firstPage().id());
    }
  }
}

void MainWindow::setOptionsWidget(FilterOptionsWidget* widget, const Ownership ownership) {
  if (isBatchProcessingInProgress()) {
    if (ownership == TRANSFER_OWNERSHIP) {
      delete widget;
    }
    return;
  }

  if (m_optionsWidget && m_optionsWidget != widget) {
    disconnectFilterOptionsWidget();
  }
  if (m_optionsWidget != widget) {
    removeWidgetsFromLayout(m_optionsFrameLayout);
  }
  // Delete the old widget we were owning, if any.
  m_optionsWidgetCleanup.clear();

  if (ownership == TRANSFER_OWNERSHIP) {
    m_optionsWidgetCleanup.add(widget);
  }

  if (m_optionsWidget == widget) {
    return;
  }

  m_optionsFrameLayout->addWidget(widget);
  m_optionsWidget = widget;

  // We use an asynchronous connection here, because the slot
  // will probably delete the options panel, which could be
  // responsible for the emission of this signal.  Qt doesn't
  // like when we delete an object while it's emitting a signal.
  connect(widget, SIGNAL(reloadRequested()), this, SLOT(reloadRequested()), Qt::QueuedConnection);
  connect(widget, SIGNAL(invalidateThumbnail(const PageId&)), this, SLOT(invalidateThumbnail(const PageId&)));
  connect(widget, SIGNAL(invalidateThumbnail(const PageInfo&)), this, SLOT(invalidateThumbnail(const PageInfo&)));
  connect(widget, SIGNAL(invalidateAllThumbnails()), this, SLOT(invalidateAllThumbnails()));
  connect(widget, SIGNAL(goToPage(const PageId&)), this, SLOT(goToPage(const PageId&)));
}  // MainWindow::setOptionsWidget

ImageViewBase* MainWindow::findPrimaryImageView(QWidget* root) {
  if (!root) {
    return nullptr;
  }
  if (auto* tabs = qobject_cast<output::TabbedImageView*>(root)) {
    QWidget* const page = tabs->currentWidget();
    if (page) {
      return Utils::castOrFindChild<ImageViewBase*>(page);
    }
    return nullptr;
  }
  return Utils::castOrFindChild<ImageViewBase*>(root);
}

void MainWindow::applySavedMainAreaViewState(ImageViewBase* view, const SavedMainAreaViewState& state) {
  if (!view) {
    return;
  }
  view->setZoomLevel(state.zoom);
  if (!state.hasScrollNorm) {
    return;
  }
  QScrollBar* const h = view->horizontalScrollBar();
  QScrollBar* const v = view->verticalScrollBar();
  const int hr = h->maximum() - h->minimum();
  const int vr = v->maximum() - v->minimum();
  if (hr <= 0 || vr <= 0) {
    return;
  }
  const double nx = qBound(0.0, state.scrollNormX, 1.0);
  const double ny = qBound(0.0, state.scrollNormY, 1.0);
  h->setValue(h->minimum() + qRound(nx * hr));
  v->setValue(v->minimum() + qRound(ny * vr));
}

void MainWindow::scheduleSavedMainAreaViewStateRestore(const QPointer<ImageViewBase>& view) {
  if (view.isNull()) {
    return;
  }
  const SavedMainAreaViewState state = m_savedMainAreaViewState;
  QTimer::singleShot(0, this, [view, state]() {
    if (view.isNull()) {
      return;
    }
    MainWindow::applySavedMainAreaViewState(view.data(), state);
  });
}

void MainWindow::setImageWidget(QWidget* widget, const Ownership ownership, DebugImages* debugImages, bool overlay) {
  if (isBatchProcessingInProgress() && (widget != m_batchProcessingWidget.get())) {
    if (ownership == TRANSFER_OWNERSHIP) {
      delete widget;
    }
    return;
  }

  if (!overlay && m_imageFrameLayout->count() > 0) {
    QWidget* oldW = m_imageFrameLayout->widget(0);
    if (ImageViewBase* oldView = findPrimaryImageView(oldW)) {
      m_savedMainAreaViewState.zoom = oldView->zoomLevel();
      QScrollBar* const h = oldView->horizontalScrollBar();
      QScrollBar* const v = oldView->verticalScrollBar();
      const int hr = h->maximum() - h->minimum();
      const int vr = v->maximum() - v->minimum();
      if (hr > 0 && vr > 0) {
        m_savedMainAreaViewState.hasScrollNorm = true;
        m_savedMainAreaViewState.scrollNormX = double(h->value() - h->minimum()) / hr;
        m_savedMainAreaViewState.scrollNormY = double(v->value() - v->minimum()) / vr;
      } else {
        m_savedMainAreaViewState.hasScrollNorm = false;
      }
    }
  }

  if (!overlay) {
    removeImageWidget();
  }

  if (ownership == TRANSFER_OWNERSHIP) {
    m_imageWidgetCleanup.add(widget);
  }

  if (!debugImages || debugImages->empty()) {
    if (widget != m_imageFrameLayout->currentWidget()) {
      m_imageFrameLayout->addWidget(widget);
      if (overlay) {
        m_imageFrameLayout->setCurrentWidget(widget);
      }
      if (ImageViewBase* newView = findPrimaryImageView(widget)) {
        scheduleSavedMainAreaViewStateRestore(QPointer<ImageViewBase>(newView));
      }
    }
  } else {
    m_tabbedDebugImages->addTab(widget, "Main");
    AutoRemovingFile file;
    QString label;
    while (!(file = debugImages->retrieveNext(&label)).get().empty()) {
      QWidget* view = new DebugImageView(file);
      m_imageWidgetCleanup.add(view);
      m_tabbedDebugImages->addTab(view, label);
    }
    m_imageFrameLayout->addWidget(m_tabbedDebugImages.get());
  }
}

void MainWindow::removeImageWidget() {
  removeWidgetsFromLayout(m_imageFrameLayout);

  m_tabbedDebugImages->clear();
  // Delete the old widget we were owning, if any.
  m_imageWidgetCleanup.clear();
}

void MainWindow::invalidateThumbnail(const PageId& pageId) {
  m_thumbSequence->invalidateThumbnail(pageId);
}

void MainWindow::invalidateThumbnail(const PageInfo& pageInfo) {
  m_thumbSequence->invalidateThumbnail(pageInfo);
}

void MainWindow::invalidateAllThumbnails() {
  m_thumbSequence->invalidateAllThumbnails();
}

std::shared_ptr<AbstractCommand<void>> MainWindow::relinkingDialogRequester() {
  class Requester : public AbstractCommand<void> {
   public:
    explicit Requester(MainWindow* wnd) : m_wnd(wnd) {}

    virtual void operator()() {
      if (!m_wnd.isNull()) {
        m_wnd->showRelinkingDialog();
      }
    }

   private:
    QPointer<MainWindow> m_wnd;
  };
  return std::make_shared<Requester>(this);
}

void MainWindow::showRelinkingDialog() {
  if (!isProjectLoaded()) {
    return;
  }

  auto* dialog = new RelinkingDialog(m_projectFile, this, m_initialDirectory);
  dialog->setAttribute(Qt::WA_DeleteOnClose);
  dialog->setWindowModality(Qt::WindowModal);

  m_pages->listRelinkablePaths(dialog->pathCollector());
  dialog->addOutputPath(RelinkablePath(m_outFileNameGen.outDir(), RelinkablePath::Dir));
  dialog->addCommonImageRoot();
  dialog->selectFirstPath();

  connect(dialog, &QDialog::accepted, [this, dialog]() { this->performRelinking(dialog->relinker()); });

  dialog->show();
}

bool MainWindow::isPathWithinDirectory(const QString& path, const QString& directory) {
  const QFileInfo pathInfo(path);
  const QFileInfo directoryInfo(directory);
  const QString absolutePath
      = pathInfo.canonicalFilePath().isEmpty() ? pathInfo.absoluteFilePath() : pathInfo.canonicalFilePath();
  const QString absoluteDirectory = directoryInfo.canonicalFilePath().isEmpty() ? directoryInfo.absoluteFilePath()
                                                                                : directoryInfo.canonicalFilePath();
  const QString relativePath = QDir::fromNativeSeparators(QDir(absoluteDirectory).relativeFilePath(absolutePath));

  return !QDir::isAbsolutePath(relativePath) && relativePath != ".." && !relativePath.startsWith("../");
}

void MainWindow::warnAboutExternalImages() {
  if (!isProjectLoaded() || m_projectFile.empty()) {
    return;
  }

  QStringList imagePaths;
  QStringList externalImages;
  const QString projectDirectory = QFileInfo(core::toQString(m_projectFile)).absolutePath();
  for (const ImageFileInfo& image : m_pages->toImageFileInfo()) {
    const QString imagePath = image.fileInfo().filePath();
    imagePaths.push_back(imagePath);
    if (!isPathWithinDirectory(imagePath, projectDirectory)) {
      externalImages.push_back(imagePath);
    }
  }
  const QString commonImageRoot = RelinkingModel::findCommonImageRoot(imagePaths);
  const QString outputDirectory = core::toQString(m_outFileNameGen.outDir());
  const bool outputOutsideImageRoot
      = !imagePaths.isEmpty()
        && (commonImageRoot.isEmpty() || !isPathWithinDirectory(outputDirectory, commonImageRoot));
  if (externalImages.isEmpty() && !outputOutsideImageRoot) {
    return;
  }

  constexpr int displayedPathLimit = 5;
  const qsizetype displayedPathCount = std::min<qsizetype>(displayedPathLimit, externalImages.size());
  QString message;
  if (!externalImages.isEmpty()) {
    message
        = tr("Some source images are outside the folder containing this project. This may be intentional, but it can "
             "mean the project is using a different image set than expected.\n\n%1 external image(s):")
              .arg(externalImages.size());
  }
  for (qsizetype i = 0; i < displayedPathCount; ++i) {
    message += QStringLiteral("\n%1").arg(QDir::toNativeSeparators(externalImages.at(i)));
  }
  if (externalImages.size() > displayedPathLimit) {
    message += tr("\n... and %1 more").arg(externalImages.size() - displayedPathLimit);
  }

  if (outputOutsideImageRoot) {
    if (!message.isEmpty()) {
      message += QStringLiteral("\n\n");
    }
    message += tr("The output directory is outside the common root of the source images. A common-root relink will "
                  "not move it together with those images.\n\nOutput directory: %1\nCommon image root: %2")
                   .arg(QDir::toNativeSeparators(outputDirectory),
                        commonImageRoot.isEmpty() ? tr("not available") : QDir::toNativeSeparators(commonImageRoot));
  }

  QMessageBox messageBox(QMessageBox::Warning, tr("Project Paths Outside Expected Roots"), message,
                         QMessageBox::NoButton, this);
  QPushButton* relinkButton = messageBox.addButton(tr("Relink..."), QMessageBox::ActionRole);
  QPushButton* continueButton = messageBox.addButton(tr("Continue"), QMessageBox::RejectRole);
  messageBox.setDefaultButton(continueButton);
  messageBox.setEscapeButton(continueButton);
  messageBox.exec();
  if (messageBox.clickedButton() == relinkButton) {
    showRelinkingDialog();
  }
}

void MainWindow::performRelinking(const std::shared_ptr<AbstractRelinker>& relinker) {
  assert(relinker);

  if (!isProjectLoaded()) {
    return;
  }

  m_pages->performRelinking(*relinker);
  m_stages->performRelinking(*relinker);
  m_outFileNameGen.performRelinking(*relinker);

  Utils::maybeCreateCacheDir(m_outFileNameGen.outDir());

  m_thumbnailCache->setThumbDir(Utils::outputDirToThumbDir(m_outFileNameGen.outDir()));
  resetThumbSequence(currentPageOrderProvider());
  m_selectedPage.set(m_thumbSequence->selectionLeader().id(), getCurrentView());

  reloadRequested();
}

void MainWindow::goFirstPage() {
  if (isBatchProcessingInProgress() || !isProjectLoaded()) {
    return;
  }

  const PageInfo firstPage(m_thumbSequence->firstPage());
  if (!firstPage.isNull()) {
    goToPage(firstPage.id());
  }
}

void MainWindow::goLastPage() {
  if (isBatchProcessingInProgress() || !isProjectLoaded()) {
    return;
  }

  const PageInfo lastPage(m_thumbSequence->lastPage());
  if (!lastPage.isNull()) {
    goToPage(lastPage.id());
  }
}

void MainWindow::goNextPage() {
  if (isBatchProcessingInProgress() || !isProjectLoaded()) {
    return;
  }

  const PageInfo nextPage(m_thumbSequence->nextPage(m_thumbSequence->selectionLeader().id()));
  if (!nextPage.isNull()) {
    goToPage(nextPage.id());
  }
}

void MainWindow::goPrevPage() {
  if (isBatchProcessingInProgress() || !isProjectLoaded()) {
    return;
  }

  const PageInfo prevPage(m_thumbSequence->prevPage(m_thumbSequence->selectionLeader().id()));
  if (!prevPage.isNull()) {
    goToPage(prevPage.id());
  }
}

void MainWindow::goNextTenPages() {
  navigatePages(-1, 10);
}

void MainWindow::goPrevTenPages() {
  navigatePages(1, 10);
}

void MainWindow::navigatePages(const int pageSteps, const int pageStepSize) {
  if (isBatchProcessingInProgress() || !isProjectLoaded()) {
    return;
  }

  const PageInfo currentPage = m_thumbSequence->selectionLeader();
  PageInfo targetPage = currentPage;
  const bool moveBackward = pageSteps > 0;
  const int pagesToMove = std::abs(pageSteps) * pageStepSize;
  for (int i = 0; i < pagesToMove && !targetPage.isNull(); ++i) {
    const PageInfo nextPage
        = moveBackward ? m_thumbSequence->prevPage(targetPage.id()) : m_thumbSequence->nextPage(targetPage.id());
    if (nextPage.isNull()) {
      break;
    }
    targetPage = nextPage;
  }
  if (!targetPage.isNull() && targetPage.id() != currentPage.id()) {
    goToPage(targetPage.id());
  }
}

void MainWindow::goNextSelectedPage() {
  if (isBatchProcessingInProgress() || !isProjectLoaded()) {
    return;
  }

  const PageInfo nextSelectedPage(m_thumbSequence->nextSelectedPage(m_thumbSequence->selectionLeader().id()));
  if (!nextSelectedPage.isNull()) {
    goToPage(nextSelectedPage.id(), ThumbnailSequence::KEEP_SELECTION);
  }
}

void MainWindow::goPrevSelectedPage() {
  if (isBatchProcessingInProgress() || !isProjectLoaded()) {
    return;
  }

  const PageInfo prevSelectedPage(m_thumbSequence->prevSelectedPage(m_thumbSequence->selectionLeader().id()));
  if (!prevSelectedPage.isNull()) {
    goToPage(prevSelectedPage.id(), ThumbnailSequence::KEEP_SELECTION);
  }
}

void MainWindow::goToPage(const PageId& pageId, const ThumbnailSequence::SelectionAction selectionAction) {
  // Do not toggle Follow page here (issue #51): keyboard navigation used to force it on while mouse
  // selection did not. Batch processing and similar flows set focus explicitly where needed.

  m_thumbSequence->setSelection(pageId, selectionAction);

  // If the page was already selected, it will be reloaded.
  // That's by design.
  updateMainArea();
}

void MainWindow::currentPageChanged(const PageInfo& pageInfo,
                                    const QRectF& thumbRect,
                                    const ThumbnailSequence::SelectionFlags flags) {
  m_selectedPage.set(pageInfo.id(), getCurrentView());

  if ((flags & ThumbnailSequence::SELECTED_BY_USER) || focusButton->isChecked()) {
    if (!(flags & ThumbnailSequence::AVOID_SCROLLING_TO)) {
      const int scrollMargin = 20;  // Keep selected thumbnail away from edge (issue #51).
      thumbView->ensureVisible(thumbRect, scrollMargin, scrollMargin);
    }
  }

  if (flags & ThumbnailSequence::SELECTED_BY_USER) {
    if (isBatchProcessingInProgress()) {
      stopBatchProcessing();
    } else if (!(flags & ThumbnailSequence::REDUNDANT_SELECTION)) {
      // Start loading / processing the newly selected page.
      updateMainArea();
    }
  }

  if ((flags & ThumbnailSequence::SELECTION_CLEARED) && selectionModeBtn->isChecked()) {
    selectionModeBtn->setChecked(false);
  }

  updateAutoSaveTimer();
}

void MainWindow::autoSaveProject() {
  if (m_projectFile.empty()) {
    return;
  }
  if (!ApplicationSettings::getInstance().isAutoSaveProjectEnabled()) {
    return;
  }

  if (m_waitingForCancelledTasks) {
    queueSave(PendingSave::AutoSave);
    return;
  }

  saveProjectWithFeedback(m_projectFile);
}

void MainWindow::pageContextMenuRequested(const PageInfo& pageInfo_, const QPoint& screenPos, bool selected) {
  if (isBatchProcessingInProgress()) {
    return;
  }
  // Make a copy to prevent it from being invalidated.
  const PageInfo pageInfo = pageInfo_;

  if (!selected) {
    goToPage(pageInfo.id());
  }

  // Parent widget helps correct multi-monitor placement (issue #75).
  QMenu menu(thumbView);

  auto& iconProvider = IconProvider::getInstance();
  QAction* insBefore = menu.addAction(iconProvider.getIcon("insert-before"), tr("Insert before ..."));
  insBefore->setToolTip(tr("Add input images before the selected page."));
  QAction* insAfter = menu.addAction(iconProvider.getIcon("insert-after"), tr("Insert after ..."));
  insAfter->setToolTip(tr("Add input images after the selected page."));

  menu.addSeparator();

  QAction* remove = menu.addAction(iconProvider.getIcon("user-trash"), tr("Remove from project ..."));
  remove->setToolTip(tr("Remove the selected pages from the project without deleting source files."));

  QAction* action = menu.exec(screenPos);
  if (action == insBefore) {
    showInsertFileDialog(BEFORE, pageInfo.imageId());
  } else if (action == insAfter) {
    showInsertFileDialog(AFTER, pageInfo.imageId());
  } else if (action == remove) {
    showRemovePagesDialog(m_thumbSequence->selectedItems());
  }
}  // MainWindow::pageContextMenuRequested

void MainWindow::pastLastPageContextMenuRequested(const QPoint& screenPos) {
  if (!isProjectLoaded()) {
    return;
  }

  QMenu menu(thumbView);
  QAction* insertHere = menu.addAction(IconProvider::getInstance().getIcon("insert-here"), tr("Insert here ..."));
  insertHere->setToolTip(tr("Add input images at the end of the project."));

  if (menu.exec(screenPos)) {
    showInsertFileDialog(BEFORE, ImageId());
  }
}

void MainWindow::thumbViewFocusToggled(const bool checked) {
  const QRectF rect(m_thumbSequence->selectionLeaderSceneRect());
  if (rect.isNull()) {
    // No selected items.
    return;
  }

  if (checked) {
    const int scrollMargin = 20;  // Follow Page: keep thumbnail visible with padding (issue #51).
    thumbView->ensureVisible(rect, scrollMargin, scrollMargin);
  }
}

void MainWindow::filterSelectionChanged(const QItemSelection& selected) {
  if (m_ignoreSelectionChanges) {
    return;
  }

  if (selected.empty()) {
    return;
  }

  const int filterIdx = selected.front().top();
  if (m_waitingForCancelledTasks) {
    m_pendingFilter = filterIdx;
    return;
  }

  if (m_batchQueue) {
    stopBatchProcessing();
    if (m_waitingForCancelledTasks) {
      m_pendingFilter = filterIdx;
      return;
    }
  }

  m_interactiveQueue->cancelAndClear();
  m_reconcileContent = m_reconcileContent || m_curFilter >= m_stages->selectContentFilterIdx();
  m_pendingFilter = filterIdx;
  waitForCancelledTasks();
}  // MainWindow::filterSelectionChanged

void MainWindow::applyFilterSelection(const int filterIdx, const bool reconcileContent) {
  const bool wasBelowFixOrientation = isBelowFixOrientation(m_curFilter);
  const bool wasBelowSelectContent = isBelowSelectContent(m_curFilter);
  m_curFilter = filterIdx;
  const bool nowBelowFixOrientation = isBelowFixOrientation(m_curFilter);
  const bool nowBelowSelectContent = isBelowSelectContent(m_curFilter);

  m_stages->filterAt(m_curFilter)->selected();

  updateSortOptions();

  // Propagate page orientations (that might have changed) to the "Split Pages" stage.
  if (!wasBelowFixOrientation && nowBelowFixOrientation) {
    // IMPORTANT: this needs to go before resetting thumbnails,
    // because it may affect them.
    if (m_pageOrientationPropagator) {
      m_pageOrientationPropagator->propagate(*m_pages);
    }  // Otherwise probably no project is loaded.
  }
  // Propagate context boxes after orientations have established the page sequence.
  if (reconcileContent || (!wasBelowSelectContent && nowBelowSelectContent)) {
    if (m_contentBoxPropagator) {
      m_contentBoxPropagator->propagate(*m_pages);
    }
  } else if (m_contentBoxPropagator && !m_dirtyContentImages.empty()) {
    m_contentBoxPropagator->propagateImages(*m_pages, m_dirtyContentImages);
  }
  m_dirtyContentImages.clear();

  const int horScrollBarPos = thumbView->horizontalScrollBar()->value();
  const int verScrollBarPos = thumbView->verticalScrollBar()->value();

  resetThumbSequence(currentPageOrderProvider(), ThumbnailSequence::KEEP_SELECTION);

  if (!focusButton->isChecked()) {
    thumbView->horizontalScrollBar()->setValue(horScrollBarPos);
    thumbView->verticalScrollBar()->setValue(verScrollBarPos);
  }

  updateMainArea();
}

void MainWindow::waitForCancelledTasks() {
  if (!m_waitingForCancelledTasks) {
    m_waitingForCancelledTasks = true;
    thumbView->setEnabled(false);
    m_imageFrameLayout->parentWidget()->setEnabled(false);
    removeFilterOptionsWidget();
    actionFixDpi->setEnabled(false);
    actionRelinking->setEnabled(false);
    if (m_fixDpiDialog) {
      m_fixDpiDialog->setEnabled(false);
    }
    QMainWindow::statusBar()->showMessage(tr("Waiting for cancelled tasks to finish..."));
  }
  QTimer::singleShot(0, this, &MainWindow::finishWaitingForCancelledTasks);
}

void MainWindow::finishWaitingForCancelledTasks() {
  // TODO: If a cancelled worker remains blocked in I/O, it cannot report completion and this wait can persist
  // indefinitely.
  if (!m_waitingForCancelledTasks || m_finishingCancelledTasks || !m_workerThreadPool->isIdle()) {
    return;
  }

  m_finishingCancelledTasks = true;
  const int filterIdx = m_pendingFilter < 0 ? m_curFilter : m_pendingFilter;
  m_pendingFilter = -1;
  const bool reconcileContent = m_reconcileContent;
  m_reconcileContent = false;
  applyFilterSelection(filterIdx, reconcileContent);

  while (!m_pendingSaves.empty()) {
    const PendingSave pendingSave = m_pendingSaves.front();
    m_pendingSaves.pop_front();
    if (pendingSave == PendingSave::Save) {
      saveProjectInteractive();
    } else if (pendingSave == PendingSave::SaveAs) {
      saveProjectAsNow();
    } else if (pendingSave == PendingSave::AutoSave) {
      if (!m_projectFile.empty() && ApplicationSettings::getInstance().isAutoSaveProjectEnabled()) {
        saveProjectWithFeedback(m_projectFile);
      }
    }
  }

  m_waitingForCancelledTasks = false;
  m_finishingCancelledTasks = false;
  thumbView->setEnabled(true);
  m_imageFrameLayout->parentWidget()->setEnabled(true);
  if (m_fixDpiDialog) {
    m_fixDpiDialog->setEnabled(true);
  }
  updateProjectActions();
  QMainWindow::statusBar()->clearMessage();
  updateMainArea();
}

void MainWindow::switchFilter1() {
  filterList->selectRow(0);
}

void MainWindow::switchFilter2() {
  filterList->selectRow(1);
}

void MainWindow::switchFilter3() {
  filterList->selectRow(2);
}

void MainWindow::switchFilter4() {
  filterList->selectRow(3);
}

void MainWindow::switchFilter5() {
  filterList->selectRow(4);
}

void MainWindow::switchFilter6() {
  filterList->selectRow(5);
}

void MainWindow::pageOrderingChanged(int idx) {
  if (m_ignorePageOrderingChanges) {
    return;
  }

  const int horScrollBarPos = thumbView->horizontalScrollBar()->value();
  const int verScrollBarPos = thumbView->verticalScrollBar()->value();

  m_stages->filterAt(m_curFilter)->selectPageOrder(idx);

  m_thumbSequence->reset(currentPageSequence(), ThumbnailSequence::KEEP_SELECTION, currentPageOrderProvider());

  if (!focusButton->isChecked()) {
    thumbView->horizontalScrollBar()->setValue(horScrollBarPos);
    thumbView->verticalScrollBar()->setValue(verScrollBarPos);
  }
}

void MainWindow::reloadRequested() {
  // Start loading / processing the current page.
  updateMainArea();
}

void MainWindow::startBatchProcessing() {
  if (m_waitingForCancelledTasks || isBatchProcessingInProgress() || !isProjectLoaded()) {
    return;
  }

  m_batchFailures.clear();
  m_batchSkippedPages.clear();
  m_interactiveQueue->cancelAndClear();

  m_batchQueue = std::make_unique<ProcessingTaskQueue>();
  updateProjectActions();
  // Use full page sequence in current display order so all pages are processed
  // (fixes batch missing pages when e.g. order is "decreasing deviation" and
  // selection was not at the first page).
  const PageSequence sequence(m_thumbSequence->toPageSequence());
  for (const PageInfo& page : sequence) {
    for (int i = 0; i < m_stages->count(); i++) {
      m_stages->filterAt(i)->loadDefaultSettings(page);
    }
    m_batchQueue->addProcessingTask(page, createCompositeTask(page, m_curFilter, /*batch=*/true, m_debug));
  }

  focusButton->setChecked(true);

  removeFilterOptionsWidget();
  filterList->setBatchProcessingInProgress(true);
  filterList->setEnabled(false);

  BackgroundTaskPtr task(m_batchQueue->takeForProcessing());
  if (task) {
    do {
      m_workerThreadPool->submitTask(task);
      if (!m_workerThreadPool->hasSpareCapacity()) {
        break;
      }
    } while ((task = m_batchQueue->takeForProcessing()));
  } else {
    stopBatchProcessing();
  }

  PageInfo selectedPage(m_batchQueue->selectedPage());
  if (!selectedPage.isNull()) {
    m_thumbSequence->setSelection(selectedPage.id());
  }
  // Display the batch processing screen.
  updateMainArea();
}  // MainWindow::startBatchProcessing

void MainWindow::stopBatchProcessing(MainAreaAction mainArea) {
  if (!isBatchProcessingInProgress()) {
    return;
  }

  const bool interrupted = !m_batchQueue->allProcessed();
  const QStringList batchFailures(std::move(m_batchFailures));
  const QStringList batchSkippedPages(std::move(m_batchSkippedPages));
  m_batchFailures.clear();
  m_batchSkippedPages.clear();

  const PageInfo page(m_batchQueue->selectedPage());
  if (!page.isNull()) {
    m_thumbSequence->setSelection(page.id());
  }

  m_batchQueue->cancelAndClear();
  m_batchQueue.reset();
  updateProjectActions();

  filterList->setBatchProcessingInProgress(false);
  filterList->setEnabled(true);

  if (mainArea != CLEAR_MAIN_AREA && (interrupted || m_curFilter >= m_stages->selectContentFilterIdx())) {
    m_interactiveQueue->cancelAndClear();
    m_pendingFilter = m_curFilter;
    m_reconcileContent = m_reconcileContent || m_curFilter >= m_stages->selectContentFilterIdx();
    waitForCancelledTasks();
  }

  switch (mainArea) {
    case UPDATE_MAIN_AREA:
      updateMainArea();
      break;
    case CLEAR_MAIN_AREA:
      removeImageWidget();
      break;
  }

  if (!m_waitingForCancelledTasks) {
    resetThumbSequence(currentPageOrderProvider());
  }

  if ((!batchFailures.isEmpty() || !batchSkippedPages.isEmpty()) && mainArea != CLEAR_MAIN_AREA) {
    QTimer::singleShot(0, this, [this, batchFailures, batchSkippedPages]() {
      QMessageBox messageBox(QMessageBox::Warning, tr("Batch processing"),
                             tr("Batch processing finished with %1 failure(s) and %2 skipped page(s).")
                                 .arg(batchFailures.size())
                                 .arg(batchSkippedPages.size()),
                             QMessageBox::Ok, this);
      QStringList details;
      if (!batchFailures.isEmpty()) {
        details.append(tr("Failed pages:\n%1").arg(batchFailures.join(QLatin1Char('\n'))));
      }
      if (!batchSkippedPages.isEmpty()) {
        details.append(tr("Skipped pages:\n%1").arg(batchSkippedPages.join(QLatin1Char('\n'))));
      }
      messageBox.setDetailedText(details.join(QStringLiteral("\n\n")));
      messageBox.exec();
    });
  }
}

void MainWindow::filterResult(const BackgroundTaskPtr& task,
                              const FilterResultPtr& result,
                              const QString& errorMessage) {
  const bool isBatchTask = m_batchQueue && task->type() == BackgroundTask::BATCH;
  const PageInfo batchPage = isBatchTask ? m_batchQueue->pageInfoForTask(task) : PageInfo();

  // Cancelled or not, we must mark it as finished.
  m_interactiveQueue->processingFinished(task);
  if (m_batchQueue) {
    m_batchQueue->processingFinished(task);
  }

  if (isBatchProcessingInProgress()) {
    feedWorkerPool();
  }

  if (task->isCancelled()) {
    if (isBatchProcessingInProgress() && m_batchQueue->allProcessed()) {
      stopBatchProcessing();
    }
    return;
  }

  bool batchFailureRecorded = false;
  bool batchPageSkipped = false;
  QString batchPageName;
  const auto recordBatchFailure = [this, isBatchTask, &batchFailureRecorded, &batchPageName](const QString& details) {
    if (!isBatchTask || batchFailureRecorded) {
      return;
    }
    const QString pageName = batchPageName.isEmpty() ? tr("Unknown page") : batchPageName;
    m_batchFailures.append(tr("%1: %2").arg(pageName, details));
    batchFailureRecorded = true;
  };

  const auto showFailure = [this](const QString& message) {
    const QString details = message.isEmpty() ? tr("An unknown error occurred.") : message;
    setImageWidget(new ErrorWidget(tr("Background processing failed:\n%1").arg(details), Qt::PlainText),
                   TRANSFER_OWNERSHIP);
    setOptionsWidget(new FilterOptionsWidget, TRANSFER_OWNERSHIP);
  };

  try {
    if (isBatchTask) {
      batchPageName = batchPage.isNull() ? tr("Unknown page") : core::toQString(batchPage.imageId().filePath());
      if (!batchPage.isNull() && batchPage.imageId().isMultiPageFile()) {
        batchPageName += tr(" (page %1)").arg(batchPage.imageId().page());
      }
      if (!batchPage.isNull() && batchPage.id().subPage() != PageId::SINGLE_PAGE) {
        batchPageName += tr(" (%1)").arg(batchPage.id().subPageAsString());
      }

      const QString failureDescription = result ? result->failureDescription() : QString();
      const QString skippedDescription = result ? result->skippedDescription() : QString();
      if (!skippedDescription.isEmpty()) {
        m_batchSkippedPages.append(tr("%1: %2").arg(batchPageName, skippedDescription));
        batchPageSkipped = true;
      }
      if (!result || !failureDescription.isEmpty() || !errorMessage.isEmpty()) {
        const QString details = !failureDescription.isEmpty()
                                    ? failureDescription
                                    : (errorMessage.isEmpty() ? tr("The page could not be processed.") : errorMessage);
        recordBatchFailure(details);
      }
    }

    if (!result) {
      showFailure(errorMessage);
    } else {
      if (!isBatchProcessingInProgress()) {
        if (!result->filter()) {
          // Error loading file.  No special action is necessary.
        } else if (result->filter() != m_stages->filterAt(m_curFilter)) {
          // Error from one of the previous filters.
          const int idx = m_stages->findFilter(result->filter());
          assert(idx >= 0);
          m_curFilter = idx;

          ScopedIncDec<int> selectionGuard(m_ignoreSelectionChanges);
          filterList->selectRow(idx);
        }
      }

      // This needs to be done even if batch processing is taking place,
      // for instance because thumbnail invalidation is done from here.
      result->updateUI(this);
    }
  } catch (const std::bad_alloc&) {
    OutOfMemoryHandler::instance().handleOutOfMemorySituation();
  } catch (const std::exception& e) {
    qCritical("Failed to update the UI for a background task: %s", e.what());
    if (isBatchTask && !batchFailureRecorded && !batchPageSkipped) {
      try {
        recordBatchFailure(tr("Failed to update the page: %1").arg(QString::fromUtf8(e.what())));
      } catch (const std::bad_alloc&) {
        OutOfMemoryHandler::instance().handleOutOfMemorySituation();
      } catch (const std::exception& reportError) {
        qWarning("Failed to add a background task UI error to the batch summary: %s", reportError.what());
      } catch (...) {
        qWarning("Failed to add a background task UI error to the batch summary.");
      }
    }
    try {
      showFailure(QString());
    } catch (const std::bad_alloc&) {
      OutOfMemoryHandler::instance().handleOutOfMemorySituation();
    } catch (const std::exception& error) {
      qWarning("Failed to display a background task error: %s", error.what());
    } catch (...) {
      qWarning("Failed to display a background task error: unknown exception.");
    }
  } catch (...) {
    qCritical("Failed to update the UI for a background task with an unknown exception.");
    if (isBatchTask && !batchFailureRecorded && !batchPageSkipped) {
      try {
        recordBatchFailure(tr("An unknown exception occurred while updating the page."));
      } catch (const std::bad_alloc&) {
        OutOfMemoryHandler::instance().handleOutOfMemorySituation();
      } catch (const std::exception& reportError) {
        qWarning("Failed to add a background task UI error to the batch summary: %s", reportError.what());
      } catch (...) {
        qWarning("Failed to add a background task UI error to the batch summary.");
      }
    }
    try {
      showFailure(QString());
    } catch (const std::bad_alloc&) {
      OutOfMemoryHandler::instance().handleOutOfMemorySituation();
    } catch (const std::exception& error) {
      qWarning("Failed to display a background task error: %s", error.what());
    } catch (...) {
      qWarning("Failed to display a background task error: unknown exception.");
    }
  }

  if (isBatchProcessingInProgress()) {
    if (m_batchQueue->allProcessed()) {
      stopBatchProcessing();

      QApplication::alert(this);  // Flash the taskbar entry.
      if (m_checkBeepWhenFinished()) {
        SettingsStore settings;
        CompletionSound::play(settings.stringValue("main_window/external_alarm_cmd"));
      }

      if (m_selectedPage.get(getCurrentView()) == m_thumbSequence->lastPage().id()) {
        // If batch processing finished at the last page, jump to the first one.
        goFirstPage();
      }
      return;
    }

    const PageInfo page(m_batchQueue->selectedPage());
    if (!page.isNull()) {
      m_thumbSequence->setSelection(page.id());
    }
  }
}  // MainWindow::filterResult

void MainWindow::feedWorkerPool() {
  do {
    const BackgroundTaskPtr task(m_batchQueue->takeForProcessing());
    if (!task) {
      break;
    }
    m_workerThreadPool->submitTask(task);
  } while (m_workerThreadPool->hasSpareCapacity());
}

void MainWindow::debugToggled(const bool enabled) {
  m_debug = enabled;
}

void MainWindow::fixDpiDialogRequested() {
  if (isBatchProcessingInProgress() || !isProjectLoaded()) {
    return;
  }

  assert(!m_fixDpiDialog);
  m_fixDpiDialog = new FixDpiDialog(m_pages->toImageFileInfo(), this);
  m_fixDpiDialog->setAttribute(Qt::WA_DeleteOnClose);
  m_fixDpiDialog->setWindowModality(Qt::WindowModal);

  connect(m_fixDpiDialog, SIGNAL(accepted()), SLOT(fixedDpiSubmitted()));

  m_fixDpiDialog->show();
}

void MainWindow::fixedDpiSubmitted() {
  assert(m_fixDpiDialog);
  assert(m_pages);
  assert(m_thumbSequence);

  const PageInfo selectedPageBefore(m_thumbSequence->selectionLeader());

  m_pages->updateMetadataFrom(m_fixDpiDialog->files());

  // The thumbnail list also stores page metadata, including the DPI.
  m_thumbSequence->reset(currentPageSequence(), ThumbnailSequence::KEEP_SELECTION,
                         m_thumbSequence->pageOrderProvider());

  const PageInfo selectedPageAfter(m_thumbSequence->selectionLeader());

  // Reload if the current page was affected.
  // Note that imageId() isn't supposed to change - we check just in case.
  if ((selectedPageBefore.imageId() != selectedPageAfter.imageId())
      || (selectedPageBefore.metadata() != selectedPageAfter.metadata())) {
    reloadRequested();
  }

  if (m_waitingForCancelledTasks) {
    queueSave(PendingSave::Save);
    QMainWindow::statusBar()->showMessage(tr("The project will be saved when cancelled tasks finish."));
    return;
  }
  saveProjectInteractive();
}

void MainWindow::saveProjectTriggered() {
  if (m_waitingForCancelledTasks) {
    queueSave(PendingSave::Save);
    QMainWindow::statusBar()->showMessage(tr("The project will be saved when cancelled tasks finish."));
    return;
  }
  saveProjectInteractive();
}

bool MainWindow::saveProjectInteractive() {
  if (m_projectFile.empty()) {
    saveProjectAsNow();
    return !m_projectFile.empty();
  }

  if (!saveProjectWithFeedback(m_projectFile)) {
    return false;
  }
  updateWindowTitle();
  return true;
}

void MainWindow::saveProjectAsTriggered() {
  if (m_waitingForCancelledTasks) {
    queueSave(PendingSave::SaveAs);
    QMainWindow::statusBar()->showMessage(tr("The project will be saved when cancelled tasks finish."));
    return;
  }
  saveProjectAsNow();
}

void MainWindow::queueSave(PendingSave save) {
  if (save == PendingSave::AutoSave) {
    if (m_pendingSaves.empty()) {
      m_pendingSaves.push_back(save);
    }
    return;
  }
  if (m_pendingSaves.size() == 1 && m_pendingSaves.front() == PendingSave::AutoSave) {
    m_pendingSaves.clear();
  }
  m_pendingSaves.push_back(save);
}

void MainWindow::saveProjectAsNow() {
  // XXX: this function is duplicated in OutOfMemoryDialog.

  QString projectDir;
  if (!m_projectFile.empty()) {
    projectDir = QFileInfo(core::toQString(m_projectFile)).absolutePath();
  } else {
    SettingsStore settings;
    projectDir = core::toQString(m_initialDirectory);
    if (projectDir.isEmpty() || !QDir(projectDir).exists()) {
      projectDir = settings.stringValue("project/lastDir");
    }
  }

  QString projectFile(QFileDialog::getSaveFileName(this, tr("Save Project"), projectDir,
                                                   tr("Scan Tailor Projects") + " (*.ScanTailor)"));
  if (projectFile.isEmpty()) {
    return;
  }

  if (!projectFile.endsWith(".ScanTailor", Qt::CaseInsensitive)) {
    projectFile += ".ScanTailor";
  }

  const std::filesystem::path projectFilePath(core::toFileSystemPath(projectFile));
  if (saveProjectWithFeedback(projectFilePath)) {
    m_projectFile = projectFilePath;
    updateWindowTitle();

    SettingsStore settings;
    settings.setValue("project/lastDir", QFileInfo(core::toQString(m_projectFile)).absolutePath());

    RecentProjects rp;
    rp.read();
    rp.setMostRecent(m_projectFile);
    rp.write();
  }
}  // MainWindow::saveProjectAsTriggered

void MainWindow::newProject() {
  if (!closeProjectInteractive()) {
    return;
  }

  // It will delete itself when it's done.
  auto* context = new ProjectCreationContext(this, m_initialDirectory);
  connect(context, SIGNAL(done(ProjectCreationContext*)), this, SLOT(newProjectCreated(ProjectCreationContext*)));
}

void MainWindow::newProjectCreated(ProjectCreationContext* context) {
  auto pages = std::make_shared<ProjectPages>(context->files(), ProjectPages::AUTO_PAGES, context->layoutDirection());
  switchToNewProject(pages, context->outDir());
}

void MainWindow::openProject() {
  if (!closeProjectInteractive()) {
    return;
  }

  QString projectDir(core::toQString(m_initialDirectory));
  if (projectDir.isEmpty() || !QDir(projectDir).exists()) {
    projectDir = SettingsStore().stringValue("project/lastDir");
  }
  QFileDialog dialog(this, tr("Open Project"), projectDir, tr("Scan Tailor Projects") + " (*.ScanTailor)");
  dialog.setFileMode(QFileDialog::ExistingFile);
  dialog.setOption(QFileDialog::DontUseNativeDialog, true);
  if (dialog.exec() != QDialog::Accepted) {
    return;
  }
  const QString projectFile(dialog.selectedFiles().value(0));
  if (projectFile.isEmpty()) {
    // Cancelled by user.
    return;
  }

  openProject(projectFile);
}

void MainWindow::openProject(const QString& projectFile) {
  QFile file(projectFile);
  if (!file.open(QIODevice::ReadOnly)) {
    QMessageBox::warning(this, tr("Error"), tr("Unable to open the project file."));
    return;
  }

  QDomDocument doc;
  if (!doc.setContent(&file)) {
    QMessageBox::warning(this, tr("Error"), tr("The project file is broken."));
    return;
  }

  file.close();

  auto* context = new ProjectOpeningContext(this, core::toFileSystemPath(projectFile), doc);
  connect(context, SIGNAL(done(ProjectOpeningContext*)), SLOT(projectOpened(ProjectOpeningContext*)));
  context->proceed();
}

void MainWindow::reloadProject() {
  if (!isProjectLoaded() || m_projectFile.empty()) {
    return;
  }
  if (m_waitingForCancelledTasks) {
    QMessageBox::information(this, tr("Please wait"), tr("Cancelled tasks are still finishing. Please try again."));
    return;
  }
  if (isBatchProcessingInProgress()) {
    QMessageBox::information(this, tr("Please wait"), tr("Stop batch processing before reloading the project."));
    return;
  }

  updateProjectModifiedState();
  if (m_projectModified) {
    QMessageBox messageBox(QMessageBox::Warning, tr("Reload Project"),
                           tr("Discard all unsaved changes and reload the project from disk?"),
                           QMessageBox::Discard | QMessageBox::Cancel, this);
    messageBox.setDefaultButton(QMessageBox::Cancel);
    if (messageBox.exec() != QMessageBox::Discard) {
      return;
    }
  }

  openProject(core::toQString(m_projectFile));
}

void MainWindow::projectOpened(ProjectOpeningContext* context) {
  RecentProjects rp;
  rp.read();
  rp.setMostRecent(context->projectFile());
  rp.write();

  SettingsStore().setValue("project/lastDir", QFileInfo(core::toQString(context->projectFile())).absolutePath());

  switchToNewProject(context->projectReader()->pages(), context->projectReader()->outputDirectory(),
                     context->projectFile(), context->projectReader());
  warnAboutExternalImages();
}

void MainWindow::closeProject() {
  closeProjectInteractive();
}

void MainWindow::openSettingsDialog() {
  auto* dialog = new SettingsDialog(this);
  dialog->setAttribute(Qt::WA_DeleteOnClose);
  dialog->setWindowModality(Qt::WindowModal);
  connect(dialog, SIGNAL(settingsChanged()), this, SLOT(onSettingsChanged()));
  dialog->show();
}

void MainWindow::openDefaultParamsDialog() {
  auto* dialog = new DefaultParamsDialog(this);
  dialog->setAttribute(Qt::WA_DeleteOnClose);
  dialog->setWindowModality(Qt::WindowModal);
  dialog->show();
}

void MainWindow::onSettingsChanged() {
  ApplicationSettings& settings = ApplicationSettings::getInstance();
  bool needInvalidate = true;

  static_cast<Application*>(qApp)->installLanguage(settings.getLanguage());

  if (m_thumbnailCache) {
    const QSize maxThumbSize = settings.getThumbnailQuality();
    if (m_thumbnailCache->getMaxThumbSize() != maxThumbSize) {
      m_thumbnailCache->setMaxThumbSize(maxThumbSize);
    }
  }

  updateThumbnailViewMode();
  setSingleColumnViewButtonState(*thumbColumnViewBtn, settings.isSingleColumnThumbnailDisplayEnabled());

  const QSizeF maxLogicalThumbSize = settings.getMaxLogicalThumbnailSize();
  if (m_maxLogicalThumbSize != maxLogicalThumbSize) {
    m_maxLogicalThumbSize = maxLogicalThumbSize;
    updateMaxLogicalThumbSize();
    needInvalidate = false;
  }

  if (needInvalidate) {
    m_thumbSequence->invalidateAllThumbnails();
  }
}

void MainWindow::showKeyboardShortcuts() {
  QDialog dialog(this);
  dialog.setWindowTitle(tr("Keyboard Shortcuts"));

  auto* layout = new QVBoxLayout(&dialog);
  auto* browser = new QTextBrowser(&dialog);
  browser->setOpenLinks(false);
  browser->setFrameShape(QFrame::NoFrame);

  const QString borderColor = dialog.palette().color(QPalette::Mid).name();
  const QString headerColor = dialog.palette().color(QPalette::AlternateBase).name();
  QString html = "<html><body>";
  html += "<h2>" + tr("Menu access").toHtmlEscaped() + "</h2>";
  html += "<table width='100%' cellspacing='0' cellpadding='7'>";
  html += "<tr bgcolor='" + headerColor + "'><th align='left'>" + tr("Menu").toHtmlEscaped() + "</th><th align='left'>"
          + tr("Shortcut").toHtmlEscaped() + "</th></tr>";
  for (QMenu* menu : {menuFile, menuDebug, menuHelp}) {
    const QString title = menuTitleWithoutMnemonic(menu->title());
    const QString shortcut = QKeySequence::mnemonic(menu->menuAction()->text()).toString(QKeySequence::NativeText);
    html += "<tr><td style='border-bottom:1px solid " + borderColor + ";'>" + title.toHtmlEscaped()
            + "</td><td style='border-bottom:1px solid " + borderColor + ";'><b>" + shortcut.toHtmlEscaped()
            + "</b></td></tr>";
  }

  html += "</table><h2>" + tr("Keyboard shortcuts").toHtmlEscaped() + "</h2>";
  html += "<table width='100%' cellspacing='0' cellpadding='7'>";
  html += "<tr bgcolor='" + headerColor + "'><th align='left'>" + tr("Action").toHtmlEscaped()
          + "</th><th align='left'>" + tr("Shortcut").toHtmlEscaped() + "</th><th align='left'>"
          + tr("Context").toHtmlEscaped() + "</th></tr>";

  const QStringList stageNames{
      QCoreApplication::translate("fix_orientation::Filter", "Fix Orientation"),
      QCoreApplication::translate("page_split::Filter", "Split Pages"),
      QCoreApplication::translate("deskew::Filter", "Deskew & Oblique"),
      QCoreApplication::translate("select_content::Filter", "Select Content"),
      QCoreApplication::translate("page_layout::Filter", "Margins"),
      QCoreApplication::translate("output::Filter", "Output"),
  };

  QList<QAction*> actions;
  QSet<QAction*> uniqueActions;
  auto appendActions = [&actions, &uniqueActions](const QList<QAction*>& candidates) {
    for (QAction* action : candidates) {
      if (action && !uniqueActions.contains(action)) {
        uniqueActions.insert(action);
        actions.append(action);
      }
    }
  };
  appendActions(this->actions());
  appendActions(findChildren<QAction*>());
  QSet<QString> actionNames;
  for (QWidget* widget : findChildren<QWidget*>()) {
    appendActions(widget->actions());
  }
  for (QAction* action : actions) {
    if (!action->objectName().isEmpty()) {
      actionNames.insert(action->objectName());
    }
  }
  std::sort(actions.begin(), actions.end(), [](const QAction* left, const QAction* right) {
    return left->text().localeAwareCompare(right->text()) < 0;
  });

  auto appendShortcutRow
      = [&html, &borderColor](const QString& label, const QStringList& shortcuts, const QString& context) {
          html += "<tr><td style='border-bottom:1px solid " + borderColor + ";'>" + label.toHtmlEscaped()
                  + "</td><td style='border-bottom:1px solid " + borderColor + ";'><b>"
                  + shortcuts.join(", ").toHtmlEscaped() + "</b></td><td style='border-bottom:1px solid " + borderColor
                  + ";'>" + context.toHtmlEscaped() + "</td></tr>";
        };

  for (QAction* action : actions) {
    const QList<QKeySequence> shortcuts = action->shortcuts();
    if (action->isSeparator() || shortcuts.isEmpty()) {
      continue;
    }

    QString label = action->text();
    if (action->objectName().startsWith("actionSwitchFilter")) {
      bool ok = false;
      const int stageNumber = action->objectName().mid(QStringLiteral("actionSwitchFilter").size()).toInt(&ok);
      if (ok && stageNumber >= 1 && stageNumber <= stageNames.size()) {
        label = stageNames.at(stageNumber - 1);
      }
    }
    if (label.isEmpty()) {
      label = action->toolTip();
    }
    if (label.isEmpty()) {
      label = action->whatsThis();
    }
    if (label.isEmpty()) {
      label = action->objectName();
    }
    if (label.isEmpty()) {
      continue;
    }

    QStringList shortcutTexts;
    for (const QKeySequence& shortcut : shortcuts) {
      shortcutTexts.append(shortcut.toString(QKeySequence::NativeText));
    }

    QString context;
    if (action->objectName().startsWith("deskewRotate") || action->objectName().startsWith("selectContent")) {
      context = tr("Active tool");
    } else {
      switch (action->shortcutContext()) {
        case Qt::ApplicationShortcut:
          context = tr("Application");
          break;
        case Qt::WindowShortcut:
          context = tr("Main window");
          break;
        case Qt::WidgetShortcut:
        case Qt::WidgetWithChildrenShortcut:
          context = tr("Active tool");
          break;
      }
    }

    appendShortcutRow(label, shortcutTexts, context);
  }

  appendShortcutRow(tr("Save project when prompted"),
                    {QKeySequence(Qt::AltModifier | Qt::Key_S).toString(QKeySequence::NativeText)},
                    tr("Save confirmation dialog"));
  appendShortcutRow(tr("Discard changes when prompted"),
                    {QKeySequence(Qt::AltModifier | Qt::Key_D).toString(QKeySequence::NativeText)},
                    tr("Save confirmation dialog"));

  if (!actionNames.contains("deskewRotateLeft")) {
    appendShortcutRow(tr("Rotate left"), {QKeySequence(",").toString(QKeySequence::NativeText)}, tr("Active tool"));
  }
  if (!actionNames.contains("deskewRotateRight")) {
    appendShortcutRow(tr("Rotate right"), {QKeySequence(".").toString(QKeySequence::NativeText)}, tr("Active tool"));
  }
  if (!actionNames.contains("selectContentCreateBox")) {
    appendShortcutRow(tr("Create Content Box"), {QKeySequence("Ins").toString(QKeySequence::NativeText)},
                      tr("Active tool"));
  }
  if (!actionNames.contains("selectContentRemoveBox")) {
    appendShortcutRow(tr("Remove Content Box"), {QKeySequence("Delete").toString(QKeySequence::NativeText)},
                      tr("Active tool"));
  }
  html += "</table></body></html>";
  browser->setHtml(html);
  layout->addWidget(browser);

  auto* buttonBox = new QDialogButtonBox(QDialogButtonBox::Close, &dialog);
  connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
  connect(buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
  layout->addWidget(buttonBox);

  dialog.resize(640, 720);
  dialog.exec();
}

void MainWindow::exportProcessedImages() {
  if (!isProjectLoaded() || isBatchProcessingInProgress()) {
    return;
  }

  const QStringList formats = ProcessedPagesExporter::supportedImageFormats();
  const QString defaultFormat = formats.contains(QStringLiteral("webp")) ? QStringLiteral("webp") : formats.value(0);
  const QString lastFormat = SettingsStore().stringValue(QStringLiteral("settings/image_export_format"), defaultFormat);
  const int savedFormatIndex = formats.indexOf(lastFormat, 0, Qt::CaseInsensitive);

  const PageSequence pages = m_thumbSequence->toPageSequence();
  int minimumDpi = 150;
  if (pages.numPages() > 0) {
    minimumDpi = std::numeric_limits<int>::max();
    const auto& outputFilter = m_stages->outputFilter();
    for (const PageInfo& page : pages) {
      const Dpi dpi(outputFilter->outputDpi(page.id()));
      minimumDpi = std::min({minimumDpi, dpi.horizontal(), dpi.vertical()});
    }
  }

  const QString defaultBaseDirectory = core::toQString(m_outFileNameGen.outDir());
  QDialog exportDialog(this);
  exportDialog.setWindowTitle(tr("Export Processed Pages as Images"));
  auto* exportLayout = new QVBoxLayout(&exportDialog);
  auto* formLayout = new QFormLayout;
  exportLayout->addLayout(formLayout);

  auto* formatCombo = new QComboBox(&exportDialog);
  formatCombo->addItems(formats);
  formatCombo->setCurrentIndex(savedFormatIndex >= 0 ? savedFormatIndex : 0);
  formLayout->addRow(tr("Image format:"), formatCombo);

  auto* baseDirectoryRowWidget = new QWidget(&exportDialog);
  auto* baseDirectoryRow = new QHBoxLayout(baseDirectoryRowWidget);
  baseDirectoryRow->setContentsMargins(0, 0, 0, 0);
  auto* baseDirectoryEdit = new QLineEdit(defaultBaseDirectory, &exportDialog);
  auto* browseButton = new QPushButton(tr("Browse..."), &exportDialog);
  baseDirectoryRow->addWidget(baseDirectoryEdit, 1);
  baseDirectoryRow->addWidget(browseButton);
  formLayout->addRow(tr("Export path:"), baseDirectoryRowWidget);

  auto* createSubdirectoryCheck = new QCheckBox(tr("Create subdirectory:"), &exportDialog);
  createSubdirectoryCheck->setChecked(true);
  auto* subdirectoryEdit
      = new QLineEdit(QStringLiteral("%1_%2").arg(formatCombo->currentText()).arg(minimumDpi), &exportDialog);
  formLayout->addRow(createSubdirectoryCheck, subdirectoryEdit);

  bool subdirectoryNameEdited = false;
  connect(subdirectoryEdit, &QLineEdit::textEdited, &exportDialog,
          [&subdirectoryNameEdited]() { subdirectoryNameEdited = true; });
  connect(formatCombo, &QComboBox::currentTextChanged, &exportDialog,
          [subdirectoryEdit, &subdirectoryNameEdited, minimumDpi](const QString& format) {
            if (!subdirectoryNameEdited) {
              subdirectoryEdit->setText(QStringLiteral("%1_%2").arg(format).arg(minimumDpi));
            }
          });
  connect(createSubdirectoryCheck, &QCheckBox::toggled, subdirectoryEdit, &QLineEdit::setEnabled);

  auto* buttonBox = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel, &exportDialog);
  buttonBox->button(QDialogButtonBox::Save)->setText(tr("Export"));
  exportLayout->addWidget(buttonBox);
  exportDialog.setMinimumWidth(600);
  exportDialog.resize(640, exportDialog.sizeHint().height());

  connect(browseButton, &QPushButton::clicked, &exportDialog, [&]() {
    const QFileInfo currentPath(baseDirectoryEdit->text().trimmed());
    const QString initialDirectory = currentPath.isDir() ? currentPath.absoluteFilePath() : currentPath.absolutePath();
    const QString parentDirectory = QFileDialog::getExistingDirectory(&exportDialog, tr("Select Export Path"),
                                                                      initialDirectory, QFileDialog::ShowDirsOnly);
    if (!parentDirectory.isEmpty()) {
      baseDirectoryEdit->setText(parentDirectory);
    }
  });
  connect(buttonBox, &QDialogButtonBox::accepted, &exportDialog, &QDialog::accept);
  connect(buttonBox, &QDialogButtonBox::rejected, &exportDialog, &QDialog::reject);
  if (exportDialog.exec() != QDialog::Accepted) {
    return;
  }

  const QString format = formatCombo->currentText();
  if (format.isEmpty()) {
    QMessageBox::warning(this, tr("Image Export Failed"), tr("Please select an image format."));
    return;
  }

  const QString baseDirectoryText = baseDirectoryEdit->text().trimmed();
  if (baseDirectoryText.isEmpty()) {
    QMessageBox::warning(this, tr("Image Export Failed"), tr("Please specify an export path."));
    return;
  }
  const QString baseDirectory = QFileInfo(baseDirectoryText).absoluteFilePath();
  const QString subdirectoryName = subdirectoryEdit->text().trimmed();
  const bool useSubdirectory = createSubdirectoryCheck->isChecked() && !subdirectoryName.isEmpty();
  if (useSubdirectory
      && (QDir::isAbsolutePath(subdirectoryName) || subdirectoryName == QStringLiteral(".")
          || subdirectoryName == QStringLiteral("..") || subdirectoryName.contains(QLatin1Char('/'))
          || subdirectoryName.contains(QLatin1Char('\\')))) {
    QMessageBox::warning(this, tr("Image Export Failed"),
                         tr("Enter a single subdirectory name without path separators."));
    return;
  }
  const QString outputDirectory = useSubdirectory ? QDir(baseDirectory).filePath(subdirectoryName) : baseDirectory;
  if (!QDir().mkpath(outputDirectory)) {
    QMessageBox::warning(this, tr("Image Export Failed"),
                         tr("Could not create the export directory: %1").arg(outputDirectory));
    return;
  }

  SettingsStore().setValue(QStringLiteral("settings/image_export_format"), format);

  QProgressDialog progress(tr("Exporting processed pages..."), tr("Cancel"), 0, static_cast<int>(pages.numPages()),
                           this);
  progress.setWindowModality(Qt::WindowModal);
  progress.setMinimumDuration(0);

  const OutputFileNameGenerator fileNameGenerator = m_outFileNameGen;
  QString errorMessage;
  const bool success = runExporterWithProgress(
      progress,
      [&pages, &fileNameGenerator, &outputDirectory, &format](
          const ProcessedPagesExporter::ProgressCallback& progressCallback, QString* error) {
        return ProcessedPagesExporter::exportImages(pages, fileNameGenerator, outputDirectory, format, progressCallback,
                                                    error);
      },
      &errorMessage);
  if (!success) {
    QMessageBox::warning(this, tr("Image Export Failed"), errorMessage);
    return;
  }
  QMessageBox::information(this, tr("Image Export Complete"),
                           tr("Processed pages were exported to %1.").arg(outputDirectory));
}

void MainWindow::exportPagesToPdf() {
  if (!isProjectLoaded() || isBatchProcessingInProgress()) {
    return;
  }

  const PageSequence pages = m_thumbSequence->toPageSequence();
  int minimumDpi = 150;
  if (pages.numPages() > 0) {
    minimumDpi = std::numeric_limits<int>::max();
    const auto& outputFilter = m_stages->outputFilter();
    for (const PageInfo& page : pages) {
      const Dpi dpi(outputFilter->outputDpi(page.id()));
      minimumDpi = std::min({minimumDpi, dpi.horizontal(), dpi.vertical()});
    }
  }

  const QString projectName = m_projectFile.empty() ? QStringLiteral("Unnamed")
                                                    : QFileInfo(core::toQString(m_projectFile)).completeBaseName();
  const QString defaultFileName = QStringLiteral("%1_%2.pdf").arg(projectName).arg(minimumDpi);
  QString outputFilePath = QFileDialog::getSaveFileName(
      this, tr("Export Processed Pages as PDF"),
      QDir(core::toQString(m_outFileNameGen.outDir())).filePath(defaultFileName), tr("PDF files (*.pdf)"));
  if (outputFilePath.isEmpty()) {
    return;
  }
  if (!outputFilePath.endsWith(QStringLiteral(".pdf"), Qt::CaseInsensitive)) {
    outputFilePath += QStringLiteral(".pdf");
  }

  QProgressDialog progress(tr("Creating multi-page PDF..."), tr("Cancel"), 0, static_cast<int>(pages.numPages()), this);
  progress.setWindowModality(Qt::WindowModal);
  progress.setMinimumDuration(0);

  const OutputFileNameGenerator fileNameGenerator = m_outFileNameGen;
  QString errorMessage;
  const bool success = runExporterWithProgress(
      progress,
      [&pages, &fileNameGenerator, &outputFilePath](const ProcessedPagesExporter::ProgressCallback& progressCallback,
                                                    QString* error) {
        return ProcessedPagesExporter::exportPdf(pages, fileNameGenerator, outputFilePath, progressCallback, error);
      },
      &errorMessage);
  if (!success) {
    QMessageBox::warning(this, tr("PDF Export Failed"), errorMessage);
    return;
  }
  QMessageBox::information(this, tr("PDF Export Complete"),
                           tr("The multi-page PDF was saved to %1.").arg(outputFilePath));
}

void MainWindow::showSupportedImageFormats() {
  if (auto* dialog = findChild<QDialog*>(QStringLiteral("supportedImageFormatsDialog"))) {
    dialog->raise();
    dialog->activateWindow();
    return;
  }

  auto* dialog = new QDialog(this);
  dialog->setObjectName(QStringLiteral("supportedImageFormatsDialog"));
  dialog->setAttribute(Qt::WA_DeleteOnClose);
  dialog->setWindowTitle(tr("Supported Image Formats"));

  auto* layout = new QVBoxLayout(dialog);
  auto* browser = new QTextBrowser(dialog);
  browser->setOpenLinks(false);
  browser->setFrameShape(QFrame::NoFrame);

  QStringList qtFormats;
  for (const QByteArray& format : QImageReader::supportedImageFormats()) {
    const QString extension = QString::fromLatin1(format).toLower();
    if (extension != QStringLiteral("tif") && extension != QStringLiteral("tiff")) {
      qtFormats.append(QLatin1Char('.') + extension);
    }
  }
  qtFormats.removeDuplicates();
  qtFormats.sort(Qt::CaseInsensitive);

  QString html = "<html><body>";
  html += "<h3>" + tr("Input formats").toHtmlEscaped() + "</h3>";
  html += "<p>" + tr("TIFF (.tif, .tiff) is supported, including multi-page TIFF files.").toHtmlEscaped() + "</p>";
  if (!qtFormats.isEmpty()) {
    html += "<p>"
            + tr("Other supported formats depend on the Qt image plugins available in this installation: %1.")
                  .arg(qtFormats.join(", ").toHtmlEscaped())
                  .toHtmlEscaped()
            + "</p>";
  }
  html += "<p>" + tr("Only the first page is read from non-TIFF formats.").toHtmlEscaped() + "</p>";
  html += "<h3>" + tr("Output format").toHtmlEscaped() + "</h3>";
  html += "<p>"
          + tr("Processed pages are kept as TIFF for project editing. Use File > Export Processed Pages as "
               "Images to create image files in supported formats, including WebP, or File > Export Processed "
               "Pages as PDF to create a multi-page PDF.")
                .toHtmlEscaped()
          + "</p>";
  html += "<h3>" + tr("TIFF compression").toHtmlEscaped() + "</h3>";
  html += "<table width='100%' cellspacing='0' cellpadding='7'>";
  html += "<tr><th align='left'>" + tr("Output image type").toHtmlEscaped() + "</th><th align='left'>"
          + tr("Settings option").toHtmlEscaped() + "</th><th align='left'>"
          + tr("Available compression methods").toHtmlEscaped() + "</th></tr>";
  html += "<tr><td>" + tr("1-bit black and white").toHtmlEscaped() + "</td><td>" + tr("B&W Compression").toHtmlEscaped()
          + "</td><td>" + tr("None, LZW, Deflate, CCITT G4 (default)").toHtmlEscaped() + "</td></tr>";
  html += "<tr><td>" + tr("Indexed, RGB and RGBA").toHtmlEscaped() + "</td><td>"
          + tr("Color Compression").toHtmlEscaped() + "</td><td>"
          + tr("None, LZW, Deflate, JPEG (default: LZW)").toHtmlEscaped() + "</td></tr>";
  html += "</table>";
  html += "<p>" + tr("LZW and Deflate are lossless. JPEG compression is lossy.").toHtmlEscaped() + "</p>";
  html += "</body></html>";
  browser->setHtml(html);
  layout->addWidget(browser);

  auto* buttonBox = new QDialogButtonBox(QDialogButtonBox::Close, dialog);
  connect(buttonBox, &QDialogButtonBox::rejected, dialog, &QDialog::close);
  connect(buttonBox, &QDialogButtonBox::accepted, dialog, &QDialog::close);
  layout->addWidget(buttonBox);

  dialog->resize(640, 520);
  dialog->show();
}

void MainWindow::showConfigurationDialog() {
  if (auto* dialog = findChild<ConfigurationDialog*>(QStringLiteral("configurationDialog"))) {
    dialog->raise();
    dialog->activateWindow();
    return;
  }

  auto* dialog = new ConfigurationDialog(this);
  dialog->setObjectName(QStringLiteral("configurationDialog"));
  dialog->setAttribute(Qt::WA_DeleteOnClose);
  dialog->show();
}

void MainWindow::showCommandLineArguments() {
  if (auto* dialog = findChild<QDialog*>(QStringLiteral("commandLineArgumentsDialog"))) {
    dialog->raise();
    dialog->activateWindow();
    return;
  }

  auto* dialog = new QDialog(this);
  dialog->setObjectName(QStringLiteral("commandLineArgumentsDialog"));
  dialog->setAttribute(Qt::WA_DeleteOnClose);
  dialog->setWindowTitle(tr("Command Line Arguments"));

  auto* layout = new QVBoxLayout(dialog);
  auto* browser = new QTextBrowser(dialog);
  browser->setOpenLinks(false);
  browser->setFrameShape(QFrame::NoFrame);

  QString html = "<html><body>";
  html += "<p><code>" + QApplication::applicationName().toHtmlEscaped() + " [options] [path]</code></p>";
  html += "<h3>" + tr("Options").toHtmlEscaped() + "</h3><dl>";
  html += "<dt><code>-h, --help</code></dt><dd>" + tr("Display help for command line arguments.").toHtmlEscaped()
          + "</dd>";
  html += "<dt><code>-c, --config &lt;directory&gt;</code></dt><dd>"
          + tr("Store settings in this directory. The application creates config.toml directly inside it. "
               "This directory must be writable. An existing config.toml takes precedence and leaves config.ini "
               "unchanged. Otherwise, the application migrates config.ini and attempts to rename it to config.ini.bak "
               "after TOML is saved; if the backup already exists or renaming fails, config.ini remains in place.")
                .toHtmlEscaped()
          + "</dd>";
  html += "</dl><h3>" + tr("Path").toHtmlEscaped() + "</h3><dl>";
  html += "<dt><code>[path]</code></dt><dd>"
          + tr("Optional existing project file (.ScanTailor) or directory. A project file is opened; a directory "
               "is used as the initial input and output location.")
                .toHtmlEscaped()
          + "</dd></dl></body></html>";
  browser->setHtml(html);
  layout->addWidget(browser);

  auto* buttonBox = new QDialogButtonBox(QDialogButtonBox::Close, dialog);
  connect(buttonBox, &QDialogButtonBox::rejected, dialog, &QDialog::close);
  connect(buttonBox, &QDialogButtonBox::accepted, dialog, &QDialog::close);
  layout->addWidget(buttonBox);

  dialog->resize(580, 360);
  dialog->show();
}

void MainWindow::applyMenuMnemonics() {
  QSet<QChar> usedMnemonics;
  for (QMenu* menu : {menuFile, menuDebug, menuHelp}) {
    const QString title = menu->title();
    QChar currentMnemonic;
    const QString visibleTitle = menuTitleWithoutMnemonic(title, &currentMnemonic);
    if (!currentMnemonic.isNull() && !usedMnemonics.contains(currentMnemonic.toUpper())) {
      usedMnemonics.insert(currentMnemonic.toUpper());
      continue;
    }

    int mnemonicIndex = -1;
    for (int i = 0; i < visibleTitle.size(); ++i) {
      const QChar candidate = visibleTitle.at(i).toUpper();
      if (candidate.isLetter() && !usedMnemonics.contains(candidate)) {
        mnemonicIndex = i;
        usedMnemonics.insert(candidate);
        break;
      }
    }

    QString titleWithMnemonic;
    for (int i = 0; i < visibleTitle.size(); ++i) {
      if (i == mnemonicIndex) {
        titleWithMnemonic += '&';
      }
      if (visibleTitle.at(i) == '&') {
        titleWithMnemonic += "&&";
      } else {
        titleWithMnemonic += visibleTitle.at(i);
      }
    }
    menu->setTitle(titleWithMnemonic);
  }
}

void MainWindow::showAboutDialog() {
  Ui::AboutDialog ui;
  auto* dialog = new QDialog(this);
  ui.setupUi(dialog);
  ui.version->setText(QString(tr("version ")) + QString::fromUtf8(VERSION));

  QResource license(":/GPLv3.html");
  ui.licenseViewer->setHtml(QString::fromUtf8((const char*) license.data(), static_cast<int>(license.size())));

  dialog->setAttribute(Qt::WA_DeleteOnClose);
  dialog->setWindowModality(Qt::WindowModal);
  dialog->show();
}

/**
 * This function is called asynchronously, always from the main thread.
 */
void MainWindow::handleOutOfMemorySituation() {
  deleteLater();

  m_outOfMemoryDialog->setParams(m_projectFile, m_initialDirectory, m_stages, m_pages, m_selectedPage,
                                 m_outFileNameGen);

  closeProjectWithoutSaving();

  m_outOfMemoryDialog->setAttribute(Qt::WA_DeleteOnClose);
  m_outOfMemoryDialog.release()->show();
}

/**
 * Note: the removed widgets are not deleted.
 */
void MainWindow::removeWidgetsFromLayout(QLayout* layout) {
  QLayoutItem* child;
  while ((child = layout->takeAt(0))) {
    delete child;
  }
}

void MainWindow::removeFilterOptionsWidget() {
  disconnectFilterOptionsWidget();
  removeWidgetsFromLayout(m_optionsFrameLayout);
  // Delete the old widget we were owning, if any.
  m_optionsWidgetCleanup.clear();

  m_optionsWidget = nullptr;
}

void MainWindow::disconnectFilterOptionsWidget() {
  if (!m_optionsWidget) {
    return;
  }

  disconnect(m_optionsWidget, SIGNAL(reloadRequested()), this, SLOT(reloadRequested()));
  disconnect(m_optionsWidget, SIGNAL(invalidateThumbnail(const PageId&)), this,
             SLOT(invalidateThumbnail(const PageId&)));
  disconnect(m_optionsWidget, SIGNAL(invalidateThumbnail(const PageInfo&)), this,
             SLOT(invalidateThumbnail(const PageInfo&)));
  disconnect(m_optionsWidget, SIGNAL(invalidateAllThumbnails()), this, SLOT(invalidateAllThumbnails()));
  disconnect(m_optionsWidget, SIGNAL(goToPage(const PageId&)), this, SLOT(goToPage(const PageId&)));
}

void MainWindow::updateProjectActions() {
  const bool loaded = isProjectLoaded();
  const bool canExport = loaded && !isBatchProcessingInProgress();
  menuPdfToImages->setEnabled(!loaded && m_pdfHelperProcess.isNull());
  actionReloadProject->setEnabled(loaded && !m_projectFile.empty() && !m_waitingForCancelledTasks
                                  && !isBatchProcessingInProgress());
  actionSaveProject->setEnabled(loaded);
  actionSaveProjectAs->setEnabled(loaded);
  actionExportProcessedImages->setEnabled(canExport);
  actionExportPagesToPdf->setEnabled(canExport);
  actionFixDpi->setEnabled(loaded);
  actionRelinking->setEnabled(loaded);
}

void MainWindow::extractPdfImages() {
  runPdfHelper(false);
}

void MainWindow::renderPdfPages() {
  runPdfHelper(true);
}

void MainWindow::runPdfHelper(const bool renderPages) {
  if (isProjectLoaded() || m_pdfHelperProcess) {
    return;
  }

  const QString operation = renderPages ? tr("Render PDF Pages to TIFF") : tr("Extract Embedded Images");
#if defined(Q_OS_WIN)
  const QString helperName = renderPages ? QStringLiteral("render-pages.ps1") : QStringLiteral("extract-images.ps1");
  const QString scriptPath = findPdfHelperScript(helperName);
  if (scriptPath.isEmpty()) {
    QMessageBox::warning(this, tr("PDF Tools Unavailable"),
                         tr("The PDF helper scripts could not be found in the application installation."));
    return;
  }
  QString interpreter = QStandardPaths::findExecutable(QStringLiteral("powershell.exe"));
  if (interpreter.isEmpty()) {
    interpreter = QStandardPaths::findExecutable(QStringLiteral("pwsh.exe"));
  }
  if (interpreter.isEmpty()) {
    QMessageBox::warning(this, tr("PowerShell Unavailable"),
                         tr("PowerShell is required to run the PDF helper scripts."));
    return;
  }
#else
  const QString helperName = renderPages ? QStringLiteral("render-pages.sh") : QStringLiteral("extract-images.sh");
  const QString scriptPath = findPdfHelperScript(helperName);
  if (scriptPath.isEmpty()) {
    QMessageBox::warning(this, tr("PDF Tools Unavailable"),
                         tr("The PDF helper scripts could not be found in the application installation."));
    return;
  }
#endif

  const QString initialDirectory = m_initialDirectory.empty() ? QDir::homePath() : core::toQString(m_initialDirectory);
  QDialog setupDialog(this);
  setupDialog.setWindowTitle(operation);
  auto* dialogLayout = new QVBoxLayout(&setupDialog);

  auto* headerLayout = new QHBoxLayout();
  auto* headerIcon = new QLabel(&setupDialog);
  headerIcon->setPixmap(style()->standardIcon(QStyle::SP_MessageBoxQuestion).pixmap(32, 32));
  headerIcon->setAlignment(Qt::AlignTop);
  headerLayout->addWidget(headerIcon, 0, Qt::AlignTop);

  auto* headerTextLayout = new QVBoxLayout();
  auto* heading = new QLabel(operation, &setupDialog);
  QFont headingFont = heading->font();
  headingFont.setBold(true);
  headingFont.setPointSize(headingFont.pointSize() + 2);
  heading->setFont(headingFont);
  headerTextLayout->addWidget(heading);
  const QString descriptionText
      = renderPages
            ? tr("Creates one complete TIFF per PDF page, including text and vector graphics. The selected DPI "
                 "controls output detail. TIFF uses lossless LZW compression; searchable PDF text is not retained.")
            : tr("Extracts embedded images as separate files without composing complete pages. Original pixel "
                 "dimensions and formats are preserved where possible. The image-info report maps each image to "
                 "its PDF page and lists its effective DPI. Text and vector graphics are not included.");
  auto* description = new QLabel(descriptionText, &setupDialog);
  description->setWordWrap(true);
  headerTextLayout->addWidget(description);
  headerLayout->addLayout(headerTextLayout, 1);
  dialogLayout->addLayout(headerLayout);

  auto* fileGroup = new QGroupBox(tr("Input and output"), &setupDialog);
  auto* fileLayout = new QFormLayout(fileGroup);
  auto* pdfPathEdit = new QLineEdit(fileGroup);
  pdfPathEdit->setPlaceholderText(tr("Select a PDF file"));
  pdfPathEdit->setClearButtonEnabled(true);
  auto* pdfBrowseButton = new QPushButton(tr("Browse..."), fileGroup);
  auto* pdfRow = new QWidget(fileGroup);
  auto* pdfRowLayout = new QHBoxLayout(pdfRow);
  pdfRowLayout->setContentsMargins(0, 0, 0, 0);
  pdfRowLayout->addWidget(pdfPathEdit, 1);
  pdfRowLayout->addWidget(pdfBrowseButton);
  fileLayout->addRow(tr("PDF file:"), pdfRow);

  auto* outputDirectoryEdit = new QLineEdit(fileGroup);
  outputDirectoryEdit->setPlaceholderText(renderPages ? tr("Choose where to save the rendered TIFF pages")
                                                      : tr("Choose where to save the extracted images"));
  outputDirectoryEdit->setClearButtonEnabled(true);
  auto* outputBrowseButton = new QPushButton(tr("Browse..."), fileGroup);
  auto* outputRow = new QWidget(fileGroup);
  auto* outputRowLayout = new QHBoxLayout(outputRow);
  outputRowLayout->setContentsMargins(0, 0, 0, 0);
  outputRowLayout->addWidget(outputDirectoryEdit, 1);
  outputRowLayout->addWidget(outputBrowseButton);
  fileLayout->addRow(tr("Output folder:"), outputRow);
  dialogLayout->addWidget(fileGroup);

  QSpinBox* dpiSpinBox = nullptr;
  if (renderPages) {
    auto* settingsGroup = new QGroupBox(tr("Rendering settings"), &setupDialog);
    auto* dpiLayout = new QHBoxLayout(settingsGroup);
    dpiLayout->addWidget(new QLabel(tr("Resolution:"), settingsGroup));
    dpiSpinBox = new QSpinBox(settingsGroup);
    dpiSpinBox->setRange(1, 2400);
    dpiSpinBox->setValue(300);
    dpiSpinBox->setSuffix(tr(" DPI"));
    dpiSpinBox->setToolTip(tr("Higher DPI creates larger images and takes longer to render."));
    dpiLayout->addWidget(dpiSpinBox);
    auto* dpiHint = new QLabel(tr("Higher resolution preserves more detail."), settingsGroup);
    dpiLayout->addWidget(dpiHint);
    dpiLayout->addStretch();
    dialogLayout->addWidget(settingsGroup);
  }

  auto* contentsGroup = new QGroupBox(tr("Output folder contents"), &setupDialog);
  auto* contentsLayout = new QVBoxLayout(contentsGroup);
  auto* contents = new QListWidget(contentsGroup);
  contents->setAlternatingRowColors(true);
  contents->setSelectionMode(QAbstractItemView::NoSelection);
  contents->setFocusPolicy(Qt::NoFocus);
  contents->setUniformItemSizes(true);
  contents->setIconSize(QSize(16, 16));
  contents->setMinimumHeight(150);
  contentsLayout->addWidget(contents);
  dialogLayout->addWidget(contentsGroup, 1);

  auto* setupButtons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &setupDialog);
  QPushButton* startButton = setupButtons->button(QDialogButtonBox::Ok);
  startButton->setText(renderPages ? tr("Render TIFF") : tr("Extract Images"));
  startButton->setEnabled(false);
  setupButtons->button(QDialogButtonBox::Cancel)->setDefault(true);
  connect(setupButtons, &QDialogButtonBox::accepted, &setupDialog, &QDialog::accept);
  connect(setupButtons, &QDialogButtonBox::rejected, &setupDialog, &QDialog::reject);
  dialogLayout->addWidget(setupButtons);

  auto refreshOutputContents = [&]() {
    contents->clear();
    const QString outputPath = outputDirectoryEdit->text().trimmed();
    const QDir outputDirectory(outputPath);
    if (outputPath.isEmpty() || !outputDirectory.exists()) {
      contentsGroup->setTitle(tr("Output folder contents"));
      new QListWidgetItem(tr("Select an output folder to preview its contents."), contents);
      return;
    }
    if (!outputDirectory.isReadable()) {
      contentsGroup->setTitle(tr("Output folder contents"));
      new QListWidgetItem(tr("The selected output folder cannot be read."), contents);
      return;
    }

    const QFileInfoList entries
        = outputDirectory.entryInfoList(QDir::AllEntries | QDir::Hidden | QDir::System | QDir::NoDotAndDotDot,
                                        QDir::DirsFirst | QDir::Name | QDir::IgnoreCase);
    contentsGroup->setTitle(tr("Output folder contents (%1)").arg(entries.size()));
    for (const QFileInfo& entry : entries) {
      const QStyle::StandardPixmap iconType = entry.isDir() ? QStyle::SP_DirIcon : QStyle::SP_FileIcon;
      new QListWidgetItem(style()->standardIcon(iconType), entry.fileName(), contents);
    }
    if (entries.isEmpty()) {
      new QListWidgetItem(tr("The folder is empty."), contents);
    }
  };
  auto updateStartButton = [&]() {
    const QFileInfo pdfInfo(pdfPathEdit->text().trimmed());
    const QFileInfo outputInfo(outputDirectoryEdit->text().trimmed());
    startButton->setEnabled(pdfInfo.isFile() && pdfInfo.isReadable()
                            && pdfInfo.suffix().compare(QStringLiteral("pdf"), Qt::CaseInsensitive) == 0
                            && outputInfo.isDir() && outputInfo.isReadable() && outputInfo.isWritable());
  };
  bool outputDirectoryManuallySet = false;
  connect(pdfPathEdit, &QLineEdit::textChanged, &setupDialog, [&](const QString& path) {
    const QFileInfo pdfInfo(path.trimmed());
    if (!outputDirectoryManuallySet && pdfInfo.isFile()) {
      outputDirectoryEdit->setText(pdfInfo.absolutePath());
    }
    refreshOutputContents();
    updateStartButton();
  });
  connect(outputDirectoryEdit, &QLineEdit::textEdited, &setupDialog,
          [&outputDirectoryManuallySet](const QString&) { outputDirectoryManuallySet = true; });
  connect(outputDirectoryEdit, &QLineEdit::textChanged, &setupDialog, [&](const QString&) {
    refreshOutputContents();
    updateStartButton();
  });
  connect(pdfBrowseButton, &QPushButton::clicked, &setupDialog, [&]() {
    const QFileInfo currentPdf(pdfPathEdit->text().trimmed());
    const QString dialogDirectory = currentPdf.isFile() ? currentPdf.absolutePath() : initialDirectory;
    const QString selectedPath
        = QFileDialog::getOpenFileName(&setupDialog, tr("Select PDF file"), dialogDirectory, tr("PDF files (*.pdf)"));
    if (!selectedPath.isEmpty()) {
      pdfPathEdit->setText(selectedPath);
    }
  });
  connect(outputBrowseButton, &QPushButton::clicked, &setupDialog, [&]() {
    const QString dialogDirectory
        = outputDirectoryEdit->text().trimmed().isEmpty() ? initialDirectory : outputDirectoryEdit->text().trimmed();
    const QString selectedPath = QFileDialog::getExistingDirectory(&setupDialog, tr("Select output folder"),
                                                                   dialogDirectory, QFileDialog::ShowDirsOnly);
    if (!selectedPath.isEmpty()) {
      outputDirectoryManuallySet = true;
      outputDirectoryEdit->setText(selectedPath);
    }
  });
  refreshOutputContents();
  setupDialog.resize(760, 600);
  setupDialog.setSizeGripEnabled(true);
  if (setupDialog.exec() != QDialog::Accepted) {
    return;
  }

  const QString pdfPath = QFileInfo(pdfPathEdit->text().trimmed()).absoluteFilePath();
  const QString outputDirectoryPath = QDir(outputDirectoryEdit->text().trimmed()).absolutePath();
  const QString dpiArgument = renderPages ? QString::number(dpiSpinBox->value()) : QString();
  const int renderDpi = renderPages ? dpiSpinBox->value() : 0;
  const QString pdfStem = QFileInfo(pdfPath).completeBaseName();
  QSet<QString> existingOutputFiles;
  for (const QString& fileName : QDir(outputDirectoryPath).entryList(QDir::Files | QDir::NoDotAndDotDot)) {
    existingOutputFiles.insert(fileName);
  }

  struct OptimizationSummary {
    int bitonalPages = 0;
    int grayscalePages = 0;
    int indexedColorPages = 0;
    int fullColorPages = 0;
    int unchangedPages = 0;
    QStringList errors;
  };

  auto* process = new QProcess(this);
  m_pdfHelperProcess = process;
  auto* progressDialog = new QProgressDialog(
      renderPages ? tr("Rendering PDF pages...") : tr("Extracting embedded images..."), QString(), 0, 0, this);
  progressDialog->setWindowTitle(tr("PDF to Images"));
  progressDialog->setWindowModality(Qt::WindowModal);
  progressDialog->setCancelButton(nullptr);
  progressDialog->setMinimumDuration(0);
  progressDialog->setAutoClose(false);
  progressDialog->setAutoReset(false);
  progressDialog->setWindowFlag(Qt::WindowCloseButtonHint, false);
  m_pdfHelperProgressDialog = progressDialog;

  connect(process, &QProcess::errorOccurred, this, [this, process, operation](const QProcess::ProcessError error) {
    if (error != QProcess::FailedToStart || m_pdfHelperProcess != process) {
      return;
    }
    const QString errorMessage = process->errorString();
    m_pdfHelperProcess = nullptr;
    if (m_pdfHelperProgressDialog) {
      m_pdfHelperProgressDialog->close();
      m_pdfHelperProgressDialog->deleteLater();
      m_pdfHelperProgressDialog = nullptr;
    }
    process->deleteLater();
    updateProjectActions();
    QMessageBox::warning(this, tr("PDF Operation Failed"), tr("Could not start %1:\n%2").arg(operation, errorMessage));
  });
  const auto finishOperation = [this, process]() {
    m_pdfHelperProcess = nullptr;
    if (m_pdfHelperProgressDialog) {
      m_pdfHelperProgressDialog->close();
      m_pdfHelperProgressDialog->deleteLater();
      m_pdfHelperProgressDialog = nullptr;
    }
    process->deleteLater();
    updateProjectActions();
  };
  connect(
      process, &QProcess::finished, this,
      [this, process, operation, renderPages, renderDpi, pdfStem, outputDirectoryPath, existingOutputFiles,
       finishOperation](const int exitCode, const QProcess::ExitStatus exitStatus) {
        const QString output = QString::fromLocal8Bit(process->readAllStandardOutput()).trimmed();
        const QString errorOutput = QString::fromLocal8Bit(process->readAllStandardError()).trimmed();

        if (exitStatus == QProcess::NormalExit && exitCode == 0) {
          if (renderPages) {
            QStringList renderedFiles;
            const QString filePrefix = pdfStem + QLatin1Char('-');
            const QDir outputDirectory(outputDirectoryPath);
            for (const QFileInfo& entry : outputDirectory.entryInfoList(QDir::Files, QDir::Name)) {
              if (existingOutputFiles.contains(entry.fileName())
                  || !entry.fileName().startsWith(filePrefix, Qt::CaseSensitive)
                  || (entry.suffix().compare(QStringLiteral("tif"), Qt::CaseInsensitive) != 0
                      && entry.suffix().compare(QStringLiteral("tiff"), Qt::CaseInsensitive) != 0)) {
                continue;
              }
              bool pageNumberIsValid = false;
              const int pageNumber = entry.completeBaseName().mid(filePrefix.size()).toInt(&pageNumberIsValid);
              if (pageNumberIsValid && pageNumber > 0) {
                renderedFiles.append(entry.absoluteFilePath());
              }
            }

            if (renderedFiles.isEmpty()) {
              finishOperation();
              QMessageBox::warning(
                  this, tr("PDF Optimization Failed"),
                  tr("The PDF renderer completed, but no newly created page TIFFs were found in the output folder."));
              return;
            }

            if (m_pdfHelperProgressDialog) {
              m_pdfHelperProgressDialog->setLabelText(tr("Analyzing and optimizing rendered TIFF pages..."));
            }
            auto summary = std::make_shared<OptimizationSummary>();
            auto* optimizationThread = QThread::create([renderedFiles, renderDpi, summary]() {
              for (const QString& filePath : renderedFiles) {
                const auto result = TiffWriter::optimizeRenderedPdfPage(filePath, renderDpi);
                switch (result.mode) {
                  case TiffWriter::RenderedPageColorMode::Bitonal:
                    ++summary->bitonalPages;
                    break;
                  case TiffWriter::RenderedPageColorMode::Grayscale:
                    ++summary->grayscalePages;
                    break;
                  case TiffWriter::RenderedPageColorMode::IndexedColor:
                    ++summary->indexedColorPages;
                    break;
                  case TiffWriter::RenderedPageColorMode::FullColor:
                    ++summary->fullColorPages;
                    break;
                  case TiffWriter::RenderedPageColorMode::Unsupported:
                    ++summary->unchangedPages;
                    break;
                  case TiffWriter::RenderedPageColorMode::Error:
                    summary->errors.append(QFileInfo(filePath).fileName() + QStringLiteral(": ") + result.errorMessage);
                    break;
                }
              }
            });
            optimizationThread->setParent(this);
            m_pdfOptimizationThread = optimizationThread;
            connect(optimizationThread, &QThread::finished, this,
                    [this, optimizationThread, summary, renderedFiles, output, operation, finishOperation]() {
                      m_pdfOptimizationThread = nullptr;
                      optimizationThread->deleteLater();
                      finishOperation();

                      QString message = output.isEmpty() ? tr("The PDF operation completed successfully.") : output;
                      message += tr("\n\nPage storage: %1 bilevel, %2 grayscale, %3 indexed-color, "
                                    "%4 full-color; %5 unchanged.")
                                     .arg(summary->bitonalPages)
                                     .arg(summary->grayscalePages)
                                     .arg(summary->indexedColorPages)
                                     .arg(summary->fullColorPages)
                                     .arg(summary->unchangedPages);
                      if (!summary->errors.isEmpty()) {
                        message += tr("\n\nSome rendered pages could not be optimized:\n%1")
                                       .arg(summary->errors.join(QLatin1Char('\n')));
                        QMessageBox::warning(this, tr("PDF Optimization Incomplete"), message);
                      } else if (summary->bitonalPages + summary->grayscalePages + summary->indexedColorPages
                                     + summary->fullColorPages + summary->unchangedPages
                                 != renderedFiles.size()) {
                        message += tr("\nSome pages were not accounted for during optimization.");
                        QMessageBox::warning(this, tr("PDF Optimization Incomplete"), message);
                      } else {
                        QMessageBox::information(this, operation, message);
                      }
                    });
            optimizationThread->start();
            return;
          }

          finishOperation();
          const QString message = output.isEmpty() ? tr("The PDF operation completed successfully.") : output;
          QMessageBox::information(this, operation, message);
          return;
        }

        finishOperation();
        const QString detail
            = errorOutput.isEmpty() ? tr("The helper exited with code %1.").arg(exitCode) : errorOutput;
        QMessageBox::warning(this, tr("PDF Operation Failed"), detail);
      });

  progressDialog->show();
#if defined(Q_OS_WIN)
  QStringList arguments{QStringLiteral("-NoProfile"),
                        QStringLiteral("-NonInteractive"),
                        QStringLiteral("-ExecutionPolicy"),
                        QStringLiteral("Bypass"),
                        QStringLiteral("-File"),
                        scriptPath,
                        QStringLiteral("-PdfPath"),
                        pdfPath,
                        QStringLiteral("-OutputDirectory"),
                        outputDirectoryPath};
  if (renderPages) {
    arguments << QStringLiteral("-Dpi") << dpiArgument;
  }
  process->start(interpreter, arguments);
#else
  QStringList arguments;
  if (renderPages) {
    arguments << QStringLiteral("--dpi") << dpiArgument;
  }
  arguments << pdfPath << outputDirectoryPath;
  process->start(scriptPath, arguments);
#endif
  updateProjectActions();
}

bool MainWindow::isBatchProcessingInProgress() const {
  return m_batchQueue != nullptr;
}

bool MainWindow::isProjectLoaded() const {
  return !m_outFileNameGen.outDir().empty();
}

bool MainWindow::isBelowSelectContent() const {
  return isBelowSelectContent(m_curFilter);
}

bool MainWindow::isBelowSelectContent(const int filterIdx) const {
  return filterIdx > m_stages->selectContentFilterIdx();
}

bool MainWindow::isBelowFixOrientation(int filterIdx) const {
  return filterIdx > m_stages->fixOrientationFilterIdx();
}

bool MainWindow::isOutputFilter() const {
  return isOutputFilter(m_curFilter);
}

bool MainWindow::isOutputFilter(const int filterIdx) const {
  return filterIdx == m_stages->outputFilterIdx();
}

PageView MainWindow::getCurrentView() const {
  return m_stages->filterAt(m_curFilter)->getView();
}

void MainWindow::updateMainArea() {
  if (m_waitingForCancelledTasks) {
    filterList->setBatchProcessingPossible(false);
    return;
  }

  if (m_pages->numImages() == 0) {
    filterList->setBatchProcessingPossible(false);
    setDockWidgetsVisible(false);
    showNewOpenProjectPanel();
    m_statusBarPanel->clear();
  } else if (isBatchProcessingInProgress()) {
    filterList->setBatchProcessingPossible(false);
    setImageWidget(m_batchProcessingWidget.get(), KEEP_OWNERSHIP);
  } else {
    setDockWidgetsVisible(true);
    const PageInfo page(m_thumbSequence->selectionLeader());
    if (page.isNull()) {
      filterList->setBatchProcessingPossible(false);
      removeImageWidget();
      removeFilterOptionsWidget();
    } else {
      // Note that loadPageInteractive may reset it to false.
      filterList->setBatchProcessingPossible(true);
      PageSequence pageSequence = m_thumbSequence->toPageSequence();
      if (pageSequence.numPages() > 0) {
        m_statusBarPanel->updatePage(pageSequence.pageNo(page.id()) + 1, pageSequence.numPages(), page.id());
      }
      loadPageInteractive(page);
    }
  }
}

bool MainWindow::checkReadyForOutput(const PageId* ignore) const {
  return m_stages->pageLayoutFilter()->checkReadyForOutput(*m_pages, ignore);
}

void MainWindow::loadPageInteractive(const PageInfo& page) {
  assert(!isBatchProcessingInProgress());

  const PageInfo previousPage = m_interactiveQueue->pageInfoForTask(m_interactiveTask.lock());
  m_interactiveQueue->cancelAndClear();

  if (m_curFilter >= m_stages->selectContentFilterIdx() && !m_workerThreadPool->isIdle()) {
    m_pendingFilter = m_curFilter;
    if (previousPage.isNull()) {
      m_reconcileContent = true;
    } else {
      m_dirtyContentImages.insert(previousPage.imageId());
    }
    waitForCancelledTasks();
    return;
  }

  if (isOutputFilter() && !checkReadyForOutput(&page.id())) {
    filterList->setBatchProcessingPossible(false);

    const QString errText(
        tr("Output is not yet possible, as the final size"
           " of pages is not yet known.\nTo determine it,"
           " run batch processing at \"Select Content\" or"
           " \"Margins\"."));

    removeFilterOptionsWidget();
    setImageWidget(new ErrorWidget(errText), TRANSFER_OWNERSHIP);
    return;
  }

  for (int i = 0; i < m_stages->count(); i++) {
    m_stages->filterAt(i)->loadDefaultSettings(page);
  }

  if (!isBatchProcessingInProgress()) {
    if (m_imageFrameLayout->indexOf(m_processingIndicationWidget.get()) != -1) {
      m_processingIndicationWidget->processingRestartedEffect();
    }
    bool currentWidgetIsImage = (Utils::castOrFindChild<ImageViewBase*>(m_imageFrameLayout->widget(0)) != nullptr);
    m_processingIndicationWidget->setBackgroundFadeEnabled(!currentWidgetIsImage);
    setImageWidget(m_processingIndicationWidget.get(), KEEP_OWNERSHIP, nullptr, currentWidgetIsImage);
    m_stages->filterAt(m_curFilter)->preUpdateUI(this, page);
  }

  assert(m_thumbnailCache);

  m_interactiveQueue->cancelAndClear();
  const BackgroundTaskPtr task = createCompositeTask(page, m_curFilter, false, m_debug);
  m_interactiveTask = task;
  m_interactiveQueue->addProcessingTask(page, task);
  m_workerThreadPool->submitTask(m_interactiveQueue->takeForProcessing());
}  // MainWindow::loadPageInteractive

void MainWindow::updateWindowTitle() {
  QString projectName;

  if (m_projectFile.empty()) {
    projectName = tr("Unnamed");
  } else {
    projectName = QFileInfo(core::toQString(m_projectFile)).completeBaseName();
  }
  const QString version(QString::fromUtf8(VERSION));
  setWindowTitle(tr("[*] %2 - ScanTailor Advanced [%1bit]").arg(sizeof(void*) * 8).arg(projectName));
  setWindowModified(m_projectModified);
}

void MainWindow::updateProjectModifiedState() {
  bool modified = false;
  if (isProjectLoaded()) {
    if (m_projectFile.empty() || !QFileInfo::exists(core::toQString(m_projectFile))) {
      modified = true;
    } else {
      const QString snapshotTemplate = QDir(QFileInfo(core::toQString(m_projectFile)).absolutePath())
                                           .filePath(QStringLiteral(".scantailor-dirty-XXXXXX"));
      QTemporaryFile snapshot(snapshotTemplate);
      if (!snapshot.open()) {
        qWarning().noquote() << "Unable to create a temporary project snapshot:" << snapshot.errorString();
        modified = true;
      } else {
        const QString snapshotPath(snapshot.fileName());
        snapshot.close();

        ProjectWriter writer(m_pages, m_selectedPage, m_outFileNameGen);
        QString errorMessage;
        if (!writer.write(core::toFileSystemPath(snapshotPath), m_stages->filters(), &errorMessage)) {
          qWarning().noquote() << "Unable to check whether the project has unsaved changes:" << errorMessage;
          modified = true;
        } else {
          modified = !compareFiles(core::toQString(m_projectFile), snapshotPath);
        }
      }
    }
  }

  m_projectModified = modified;
  setWindowModified(m_projectModified);
}

/**
 * \brief Closes the currently project, prompting to save it if necessary.
 *
 * \return true if the project was closed, false if the user cancelled the process.
 */
bool MainWindow::closeProjectInteractive() {
  if (m_waitingForCancelledTasks) {
    QMessageBox::information(this, tr("Please wait"), tr("Cancelled tasks are still finishing. Please try again."));
    return false;
  }
  if (!isProjectLoaded()) {
    return true;
  }

  if (m_projectFile.empty()) {
    switch (promptProjectSave()) {
      case SAVE:
        if (!saveProjectInteractive()) {
          return false;
        }
        break;
      case DONT_SAVE:
        break;
      case CANCEL:
        return false;
    }
    closeProjectWithoutSaving();
    return true;
  }

  const QFileInfo projectFile(core::toQString(m_projectFile));
  const QFileInfo backupFile(projectFile.absoluteDir(), QString::fromLatin1("Backup.") + projectFile.fileName());
  const QString backupFilePath(backupFile.absoluteFilePath());

  ProjectWriter writer(m_pages, m_selectedPage, m_outFileNameGen);

  if (!writer.write(core::toFileSystemPath(backupFilePath), m_stages->filters())) {
    // Backup file could not be written???
    QFile::remove(backupFilePath);
    switch (promptProjectSave()) {
      case SAVE:
        if (!saveProjectInteractive()) {
          return false;
        }
        break;
      case DONT_SAVE:
        break;
      case CANCEL:
        return false;
    }
    closeProjectWithoutSaving();
    return true;
  }

  if (compareFiles(core::toQString(m_projectFile), backupFilePath)) {
    // The project hasn't really changed.
    QFile::remove(backupFilePath);
    closeProjectWithoutSaving();
    return true;
  }

  switch (promptProjectSave()) {
    case SAVE: {
      std::filesystem::path backupPath;
      QString errorMessage;
      if (!ProjectMigration::createBackupForFormat3(m_projectFile, &backupPath, &errorMessage)) {
        QMessageBox::warning(
            this, tr("Error"),
            tr("The original format-3 project could not be preserved, so it was not overwritten.\n\n%1")
                .arg(errorMessage));
        return false;
      }
      if (!createProjectBackup(m_projectFile, ApplicationSettings::getInstance().getProjectBackupCount(),
                               &errorMessage)) {
        QFile::remove(backupFilePath);
        QMessageBox::warning(this, tr("Error"),
                             tr("The previous project version could not be preserved, so it was not overwritten.\n\n%1")
                                 .arg(errorMessage));
        return false;
      }
    }
      if (!Utils::overwritingRename(backupFilePath, core::toQString(m_projectFile))) {
        QMessageBox::warning(this, tr("Error"), tr("Error saving the project file!"));
        return false;
      }
      // fall through
    case DONT_SAVE:
      QFile::remove(backupFilePath);
      break;
    case CANCEL:
      return false;
  }

  closeProjectWithoutSaving();
  return true;
}  // MainWindow::closeProjectInteractive

void MainWindow::closeProjectWithoutSaving() {
  auto pages = std::make_shared<ProjectPages>();
  switchToNewProject(pages, {});
}

bool MainWindow::saveProjectWithFeedback(const std::filesystem::path& projectFile) {
  ProjectWriter writer(m_pages, m_selectedPage, m_outFileNameGen);

  QString errorMessage;
  if (!writer.write(projectFile, m_stages->filters(), &errorMessage,
                    ApplicationSettings::getInstance().getProjectBackupCount())) {
    const QString message = errorMessage.isEmpty() ? tr("Error saving the project file!")
                                                   : tr("Error saving the project file!\n\n%1").arg(errorMessage);
    QMessageBox::warning(this, tr("Error"), message);
    return false;
  }
  m_projectModified = false;
  setWindowModified(false);
  return true;
}

/**
 * Note: showInsertFileDialog(BEFORE, ImageId()) is legal and means inserting at the end.
 */
void MainWindow::showInsertFileDialog(BeforeOrAfter beforeOrAfter, const ImageId& existing) {
  if (isBatchProcessingInProgress() || !isProjectLoaded()) {
    return;
  }
  // We need to filter out files already in project (issue #88: use normalized paths so
  // dialog and project paths compare correctly when project already has images).
  class ProxyModel : public QSortFilterProxyModel {
   public:
    explicit ProxyModel(const ProjectPages& pages) {
      setDynamicSortFilter(true);

      const PageSequence sequence(pages.toPageSequence(IMAGE_VIEW));
      for (const PageInfo& page : sequence) {
        const QString path(QFileInfo(core::toQString(page.imageId().filePath())).absoluteFilePath());
        if (!path.isEmpty()) {
          m_inProjectPaths.insert(QDir::cleanPath(path));
        }
      }
    }

   protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const override {
      const QModelIndex idx = sourceModel()->index(sourceRow, 0, sourceParent);
      const QVariant data(idx.data(QFileSystemModel::FilePathRole));
      if (data.isNull()) {
        return true;
      }
      const QString path(QDir::cleanPath(QFileInfo(data.toString()).absoluteFilePath()));
      return !m_inProjectPaths.contains(path);
    }

    bool lessThan(const QModelIndex& left, const QModelIndex& right) const override { return left.row() < right.row(); }

   private:
    QSet<QString> m_inProjectPaths;
  };

  QString dialogDir;
  if (existing.isNull()) {
    dialogDir = m_projectFile.empty() ? core::toQString(m_initialDirectory)
                                      : QFileInfo(core::toQString(m_projectFile)).absolutePath();
  } else {
    dialogDir = QFileInfo(existing.filePath()).absolutePath();
    if (dialogDir.isEmpty()) {
      dialogDir = m_projectFile.empty() ? core::toQString(m_initialDirectory)
                                        : QFileInfo(core::toQString(m_projectFile)).absolutePath();
    }
  }
  if (dialogDir.isEmpty() || !QDir(dialogDir).exists()) {
    dialogDir = QDir::homePath();
  }
  auto dialog = std::make_unique<QFileDialog>(this, tr("Files to insert"), dialogDir);
  dialog->setFileMode(QFileDialog::ExistingFiles);
  dialog->setOption(QFileDialog::DontUseNativeDialog, true);
  dialog->setProxyModel(new ProxyModel(*m_pages));
  dialog->setNameFilter(tr("Images not in project (%1)").arg("*.png *.tiff *.tif *.jpeg *.jpg"));
  // XXX: Adding individual pages from a multi-page TIFF where
  // some of the pages are already in project is not supported right now.
  if (dialog->exec() != QDialog::Accepted) {
    return;
  }

  QStringList files(dialog->selectedFiles());
  if (files.empty()) {
    return;
  }

  // The order of items returned by QFileDialog is platform-dependent,
  // so we enforce our own ordering.
  std::sort(files.begin(), files.end(), SmartFilenameOrdering());

  // I suspect on some platforms it may be possible to select the same file twice,
  // so to be safe, remove duplicates.
  files.erase(std::unique(files.begin(), files.end()), files.end());


  std::vector<ImageFileInfo> newFiles;
  std::vector<QString> loadedFiles;
  std::vector<QString> failedFiles;  // Those we failed to read metadata from.
  // dialog->selectedFiles() returns file list in reverse order.
  for (int i = files.size() - 1; i >= 0; --i) {
    const QFileInfo fileInfo(files[i]);
    ImageFileInfo imageFileInfo(fileInfo, std::vector<ImageMetadata>());

    const ImageMetadataLoader::Status status = ImageMetadataLoader::load(
        core::toFileSystemPath(files.at(i)),
        [&](const ImageMetadata& metadata) { imageFileInfo.imageInfo().push_back(metadata); });

    if (status == ImageMetadataLoader::LOADED) {
      newFiles.push_back(imageFileInfo);
      loadedFiles.push_back(fileInfo.absoluteFilePath());
    } else {
      failedFiles.push_back(fileInfo.absoluteFilePath());
    }
  }

  if (!failedFiles.empty()) {
    auto errDialog = std::make_unique<LoadFilesStatusDialog>(this);
    errDialog->setLoadedFiles(loadedFiles);
    errDialog->setFailedFiles(failedFiles);
    errDialog->setOkButtonName(QString(" %1 ").arg(tr("Skip failed files")));
    if ((errDialog->exec() != QDialog::Accepted) || loadedFiles.empty()) {
      return;
    }
  }

  // Check if there is at least one DPI that's not OK.
  if (std::find_if(newFiles.begin(), newFiles.end(), [&](const ImageFileInfo& p) -> bool { return !p.isDpiOK(); })
      != newFiles.end()) {
    auto dpiDialog = std::make_unique<FixDpiDialog>(newFiles, this);
    dpiDialog->setWindowModality(Qt::WindowModal);
    if (dpiDialog->exec() != QDialog::Accepted) {
      return;
    }

    newFiles = dpiDialog->files();
  }

  // Actually insert the new pages.
  for (const ImageFileInfo& file : newFiles) {
    int imageNum = -1;  // Zero-based image number in a multi-page TIFF.
    for (const ImageMetadata& metadata : file.imageInfo()) {
      ++imageNum;

      const int numSubPages = ProjectPages::adviseNumberOfLogicalPages(metadata, OrthogonalRotation());
      const ImageInfo imageInfo(ImageId(core::toFileSystemPath(file.fileInfo().absoluteFilePath()), imageNum), metadata,
                                numSubPages, false, false);
      insertImage(imageInfo, beforeOrAfter, existing);
    }
  }
}  // MainWindow::showInsertFileDialog

void MainWindow::showRemovePagesDialog(const std::set<PageId>& pages) {
  auto dialog = std::make_unique<QDialog>(this);
  Ui::RemovePagesDialog ui;
  ui.setupUi(dialog.get());
  ui.icon->setPixmap(style()->standardIcon(QStyle::SP_MessageBoxQuestion).pixmap(48, 48));

  ui.text->setText(ui.text->text().arg(pages.size()));

  QPushButton* removeBtn = ui.buttonBox->button(QDialogButtonBox::Ok);
  removeBtn->setText(tr("Remove"));

  dialog->setWindowModality(Qt::WindowModal);
  if (dialog->exec() == QDialog::Accepted) {
    removeFromProject(pages);
    eraseOutputFiles(pages);
  }
}

/**
 * Note: insertImage(..., BEFORE, ImageId()) is legal and means inserting at the end.
 */
void MainWindow::insertImage(const ImageInfo& newImage, BeforeOrAfter beforeOrAfter, ImageId existing) {
  std::vector<PageInfo> pages(m_pages->insertImage(newImage, beforeOrAfter, existing, getCurrentView()));

  if (beforeOrAfter == BEFORE) {
    // The second one will be inserted first, then the first
    // one will be inserted BEFORE the second one.
    std::reverse(pages.begin(), pages.end());
  }

  for (const PageInfo& pageInfo : pages) {
    m_outFileNameGen.disambiguator()->registerFile(pageInfo.imageId().filePath());
    m_thumbSequence->insert(pageInfo, beforeOrAfter, existing);
    existing = pageInfo.imageId();
  }
}

void MainWindow::removeFromProject(const std::set<PageId>& pages) {
  m_interactiveQueue->cancelAndRemove(pages);
  if (m_batchQueue) {
    m_batchQueue->cancelAndRemove(pages);
  }

  m_pages->removePages(pages);


  const PageSequence itemsInOrder = m_thumbSequence->toPageSequence();
  std::set<PageId> newSelection;

  bool selectFirstNonDeleted = false;
  if (itemsInOrder.numPages() > 0) {
    // if first page was deleted select first not deleted page
    // otherwise select last not deleted page from beginning
    selectFirstNonDeleted = pages.find(itemsInOrder.pageAt(0).id()) != pages.end();

    PageId lastNonDeleted;
    for (const PageInfo& page : itemsInOrder) {
      const PageId& id = page.id();
      const bool wasDeleted = (pages.find(id) != pages.end());

      if (!wasDeleted) {
        if (selectFirstNonDeleted) {
          m_thumbSequence->setSelection(id);
          newSelection.insert(id);
          break;
        } else {
          lastNonDeleted = id;
        }
      } else if (!selectFirstNonDeleted && !lastNonDeleted.isNull()) {
        m_thumbSequence->setSelection(lastNonDeleted);
        newSelection.insert(lastNonDeleted);
        break;
      }
    }

    m_thumbSequence->removePages(pages);

    if (newSelection.empty()) {
      // fallback to old behaviour
      if (m_thumbSequence->selectionLeader().isNull()) {
        m_thumbSequence->setSelection(m_thumbSequence->firstPage().id());
      }
    }
  }

  updateMainArea();
}  // MainWindow::removeFromProject

void MainWindow::eraseOutputFiles(const std::set<PageId>& pages) {
  std::vector<PageId::SubPage> eraseVariations;
  eraseVariations.reserve(3);

  for (const PageId& pageId : pages) {
    eraseVariations.clear();
    switch (pageId.subPage()) {
      case PageId::SINGLE_PAGE:
        eraseVariations.push_back(PageId::SINGLE_PAGE);
        eraseVariations.push_back(PageId::LEFT_PAGE);
        eraseVariations.push_back(PageId::RIGHT_PAGE);
        break;
      case PageId::LEFT_PAGE:
        eraseVariations.push_back(PageId::SINGLE_PAGE);
        eraseVariations.push_back(PageId::LEFT_PAGE);
        break;
      case PageId::RIGHT_PAGE:
        eraseVariations.push_back(PageId::SINGLE_PAGE);
        eraseVariations.push_back(PageId::RIGHT_PAGE);
        break;
    }

    for (PageId::SubPage subpage : eraseVariations) {
      QFile::remove(core::toQString(m_outFileNameGen.filePathFor(PageId(pageId.imageId(), subpage))));
    }
  }
}

BackgroundTaskPtr MainWindow::createCompositeTask(const PageInfo& page,
                                                  const int lastFilterIdx,
                                                  const bool batch,
                                                  bool debug) {
  std::shared_ptr<fix_orientation::Task> fixOrientationTask;
  std::shared_ptr<page_split::Task> pageSplitTask;
  std::shared_ptr<deskew::Task> deskewTask;
  std::shared_ptr<select_content::Task> selectContentTask;
  std::shared_ptr<page_layout::Task> pageLayoutTask;
  std::shared_ptr<output::Task> outputTask;

  if (batch) {
    debug = false;
  }

  if (lastFilterIdx >= m_stages->outputFilterIdx()) {
    outputTask = m_stages->outputFilter()->createTask(page.id(), m_thumbnailCache, m_outFileNameGen, batch, debug);
    debug = false;
  }
  if (lastFilterIdx >= m_stages->pageLayoutFilterIdx()) {
    pageLayoutTask = m_stages->pageLayoutFilter()->createTask(page.id(), outputTask, batch, debug);
    debug = false;
  }
  if (lastFilterIdx >= m_stages->selectContentFilterIdx()) {
    selectContentTask = m_stages->selectContentFilter()->createTask(page.id(), pageLayoutTask, batch, debug);
    debug = false;
  }
  if (lastFilterIdx >= m_stages->deskewFilterIdx()) {
    deskewTask = m_stages->deskewFilter()->createTask(page.id(), selectContentTask, batch, debug);
    debug = false;
  }
  if (lastFilterIdx >= m_stages->pageSplitFilterIdx()) {
    pageSplitTask = m_stages->pageSplitFilter()->createTask(page, deskewTask, batch, debug);
    debug = false;
  }
  if (lastFilterIdx >= m_stages->fixOrientationFilterIdx()) {
    fixOrientationTask = m_stages->fixOrientationFilter()->createTask(page.id(), pageSplitTask, batch);
    debug = false;
  }
  assert(fixOrientationTask);
  return std::make_shared<LoadFileTask>(batch ? BackgroundTask::BATCH : BackgroundTask::INTERACTIVE, page,
                                        m_thumbnailCache, m_pages, fixOrientationTask);
}  // MainWindow::createCompositeTask

std::shared_ptr<CompositeCacheDrivenTask> MainWindow::createCompositeCacheDrivenTask(const int lastFilterIdx) {
  std::shared_ptr<fix_orientation::CacheDrivenTask> fixOrientationTask;
  std::shared_ptr<page_split::CacheDrivenTask> pageSplitTask;
  std::shared_ptr<deskew::CacheDrivenTask> deskewTask;
  std::shared_ptr<select_content::CacheDrivenTask> selectContentTask;
  std::shared_ptr<page_layout::CacheDrivenTask> pageLayoutTask;
  std::shared_ptr<output::CacheDrivenTask> outputTask;

  if (lastFilterIdx >= m_stages->outputFilterIdx()) {
    outputTask = m_stages->outputFilter()->createCacheDrivenTask(m_outFileNameGen);
  }
  if (lastFilterIdx >= m_stages->pageLayoutFilterIdx()) {
    pageLayoutTask = m_stages->pageLayoutFilter()->createCacheDrivenTask(outputTask);
  }
  if (lastFilterIdx >= m_stages->selectContentFilterIdx()) {
    selectContentTask = m_stages->selectContentFilter()->createCacheDrivenTask(pageLayoutTask);
  }
  if (lastFilterIdx >= m_stages->deskewFilterIdx()) {
    deskewTask = m_stages->deskewFilter()->createCacheDrivenTask(selectContentTask);
  }
  if (lastFilterIdx >= m_stages->pageSplitFilterIdx()) {
    pageSplitTask = m_stages->pageSplitFilter()->createCacheDrivenTask(deskewTask);
  }
  if (lastFilterIdx >= m_stages->fixOrientationFilterIdx()) {
    fixOrientationTask = m_stages->fixOrientationFilter()->createCacheDrivenTask(pageSplitTask);
  }

  assert(fixOrientationTask);
  return fixOrientationTask;
}  // MainWindow::createCompositeCacheDrivenTask

void MainWindow::updateDisambiguationRecords(const PageSequence& pages) {
  for (const PageInfo& page : pages) {
    m_outFileNameGen.disambiguator()->registerFile(page.imageId().filePath());
  }
}

PageSelectionAccessor MainWindow::newPageSelectionAccessor() {
  auto provider = std::make_shared<PageSelectionProviderImpl>(this);
  return PageSelectionAccessor(provider);
}

void MainWindow::changeEvent(QEvent* event) {
  if (event != nullptr) {
    switch (event->type()) {
      case QEvent::LanguageChange:
        retranslateUi(this);
        applyMenuMnemonics();
        updateWindowTitle();
        break;
      default:
        QWidget::changeEvent(event);
        break;
    }
  }
}

void MainWindow::setDockWidgetsVisible(bool state) {
  filterDockWidget->setVisible(state);
  thumbnailsDockWidget->setVisible(state);
}

void MainWindow::scaleThumbnails(const double scaleFactor, const bool anchorUnderMouse) {
  const QSizeF oldSize(m_maxLogicalThumbSize);
  const double dx = 25.0 * scaleFactor;
  const double dy = 16.0 * scaleFactor;
  const double width = qBound(100.0, m_maxLogicalThumbSize.width() + dx, 1000.0);
  const double height = qBound(64.0, m_maxLogicalThumbSize.height() + dy, 640.0);
  if ((width == oldSize.width()) && (height == oldSize.height())) {
    return;
  }

  const double scale = std::sqrt((width / oldSize.width()) * (height / oldSize.height()));
  const QGraphicsView::ViewportAnchor oldAnchor = thumbView->transformationAnchor();
  thumbView->setTransformationAnchor(anchorUnderMouse ? QGraphicsView::AnchorUnderMouse
                                                      : QGraphicsView::AnchorViewCenter);
  thumbView->scale(scale, scale);
  thumbView->setTransformationAnchor(oldAnchor);

  m_maxLogicalThumbSize = QSizeF(width, height);
  m_maxLogicalThumbSizeUpdater.start(350);
  ApplicationSettings::getInstance().setMaxLogicalThumbnailSize(m_maxLogicalThumbSize);
}

void MainWindow::updateMaxLogicalThumbSize() {
  thumbView->resetTransform();
  m_thumbSequence->setMaxLogicalThumbSize(m_maxLogicalThumbSize);
  updateThumbViewMinWidth();
  resetThumbSequence(currentPageOrderProvider(), ThumbnailSequence::KEEP_SELECTION);
}

void MainWindow::setupIcons() {
  auto& iconProvider = IconProvider::getInstance();
  focusButton->setIcon(iconProvider.getIcon("eye"));
  prevPageBtn->setIcon(iconProvider.getIcon("triangle-up-arrow"));
  nextPageBtn->setIcon(iconProvider.getIcon("triangle-down-arrow"));
  filterSelectedBtn->setIcon(iconProvider.getIcon("check-mark"));
  gotoPageBtn->setIcon(iconProvider.getIcon("right-pointing"));
  selectionModeBtn->setIcon(iconProvider.getIcon("checkbox-styled"));
  thumbColumnViewBtn->setIcon(iconProvider.getIcon("column-view"));
  pauseSortingBtn->setIcon(iconProvider.getIcon("stop"));
  sortingOrderBtn->setIcon(iconProvider.getIcon("sorting-order"));
  deviationHighlightingBtn->setIcon(iconProvider.getIcon("six-spoked-asterisk"));
  diminishThumbnailsBtn->setIcon(iconProvider.getIcon("diminishing-glass"));
  magnifyThumbnailsBtn->setIcon(iconProvider.getIcon("magnifying-glass"));
}

void MainWindow::execGotoPageDialog() {
  if (isBatchProcessingInProgress() || !isProjectLoaded()) {
    return;
  }

  bool ok;
  const PageSequence pageSequence = m_thumbSequence->toPageSequence();
  const PageId selectionLeader = m_thumbSequence->selectionLeader().id();
  int pageNumber
      = QInputDialog::getInt(this, tr("Go To Page"), tr("Enter the page number:"),
                             pageSequence.pageNo(selectionLeader) + 1, 1, (int) (pageSequence.numPages()), 1, &ok);
  if (ok) {
    const PageId newSelectionLeader = pageSequence.pageAt(pageNumber - 1).id();
    if (selectionLeader != newSelectionLeader) {
      goToPage(newSelectionLeader);
    }
  }
}

void MainWindow::updateThumbnailViewMode() {
  const ThumbnailSequence::ViewMode viewMode
      = (ApplicationSettings::getInstance().isSingleColumnThumbnailDisplayEnabled()) ? ThumbnailSequence::SINGLE_COLUMN
                                                                                     : ThumbnailSequence::MULTI_COLUMN;
  if (m_thumbSequence->getViewMode() != viewMode) {
    m_thumbSequence->setViewMode(viewMode);
  }
}

void MainWindow::updateAutoSaveTimer() {
  if (m_autoSaveTimer.remainingTime() <= 0) {
    m_autoSaveTimer.start(60000);
  }
}

PageSequence MainWindow::currentPageSequence() {
  PageSequence pageSequence = m_pages->toPageSequence(getCurrentView());
  if (sortingOrderBtn->isChecked()) {
    std::reverse(pageSequence.begin(), pageSequence.end());
  }
  return pageSequence;
}

void MainWindow::reloadCurrentPage() {
  if (isBatchProcessingInProgress() || !isProjectLoaded())
    return;

  updateMainArea();
}
