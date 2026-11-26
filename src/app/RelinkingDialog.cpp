// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "RelinkingDialog.h"

#include <core/IconProvider.h>
#include <core/PathUtils.h>

#include <QApplication>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QHash>
#include <QItemSelectionModel>
#include <QPalette>
#include <QSignalBlocker>
#include <QStandardItem>
#include <QStyle>
#include <QTreeView>
#include <QVector>
#include <cassert>

#include "RelinkingSortingModel.h"

namespace {

bool isPathWithinDirectory(const QString& path, const QString& directory) {
  const QString relativePath = QDir::fromNativeSeparators(QDir(directory).relativeFilePath(path));
  return relativePath == "."
         || (!QDir::isAbsolutePath(relativePath) && relativePath != ".." && !relativePath.startsWith("../"));
}

}  // namespace

RelinkingDialog::RelinkingDialog(const QString& projectFilePath, QWidget* parent, const QString& initialDirectory)
    : QDialog(parent),
      m_treeModel(new QStandardItemModel(this)),
      m_sortingModel(new RelinkingSortingModel),
      m_projectFileDir(QFileInfo(projectFilePath).absolutePath()) {
  if (m_projectFileDir.isEmpty() || !QDir(m_projectFileDir).exists()) {
    m_projectFileDir = initialDirectory;
  }
  if (m_projectFileDir.isEmpty() || !QDir(m_projectFileDir).exists()) {
    m_projectFileDir = QDir::homePath();
  }
  ui.setupUi(this);
  ui.undoButton->setIcon(IconProvider::getInstance().getIcon("undo"));
  ui.undoButton->setIconSize(QSize(16, 16));
  m_sortingModel->setSourceModel(m_treeModel);
  ui.listView->setModel(m_sortingModel);
  ui.listView->setTextElideMode(Qt::ElideMiddle);
  ui.listView->setHeaderHidden(true);
  ui.listView->setUniformRowHeights(true);
  ui.listView->setExpandsOnDoubleClick(true);
  ui.errorLabel->setVisible(false);
  ui.undoButton->setVisible(false);
  ui.useConfigPathButton->setEnabled(false);
  ui.summaryLabel->setTextFormat(Qt::RichText);
  updateConfigPathButtonAppearance();
  m_summaryUpdateTimer.setSingleShot(true);
  connect(&m_summaryUpdateTimer, &QTimer::timeout, this, &RelinkingDialog::updateSummary);
  updateSummary();

  connect(ui.listView->selectionModel(), SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)),
          SLOT(selectionChanged(const QItemSelection&, const QItemSelection&)));

  connect(&m_model, &QAbstractItemModel::dataChanged, this, [this](const QModelIndex& first, const QModelIndex& last) {
    updateTreeItems(first, last);
    updateConfigPathButtonAppearance();
    scheduleSummaryUpdate();
  });
  connect(&m_model, &QAbstractItemModel::rowsInserted, this, [this]() {
    updateConfigPathButtonAppearance();
    if (m_treeBuilt) {
      rebuildTreeModel();
      scheduleSummaryUpdate();
    }
  });

  connect(ui.pathVisualization, SIGNAL(clicked(const QString&, const QString&, int)),
          SLOT(pathButtonClicked(const QString&, const QString&, int)));

  connect(ui.undoButton, SIGNAL(clicked()), SLOT(undoButtonClicked()));
  connect(ui.useConfigPathButton, SIGNAL(clicked()), SLOT(useConfigPath()));

  disconnect(ui.buttonBox, SIGNAL(accepted()));
  connect(ui.buttonBox, SIGNAL(accepted()), SLOT(commitChanges()));
}

void RelinkingDialog::selectFirstPath() {
  if (!m_treeBuilt) {
    rebuildTreeModel();
    updateSummary();
  }
  const QModelIndex firstIndex(firstSelectableIndex());
  if (firstIndex.isValid()) {
    ui.listView->selectionModel()->setCurrentIndex(firstIndex,
                                                   QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
  }
}

void RelinkingDialog::selectionChanged(const QItemSelection& selected, const QItemSelection& deselected) {
  if (selected.isEmpty()) {
    ui.useConfigPathButton->setEnabled(false);
    ui.pathVisualization->clear();
    ui.pathVisualization->setVisible(false);
  } else {
    ui.undoButton->setVisible(false);

    QModelIndex index(selected.front().topLeft());
    const int sourceRow = index.data(m_model.TreeSourceRowRole).toInt();
    const bool rollback = ui.errorLabel->isVisible();
    if (rollback) {
      m_model.rollbackChanges();
      rebuildTreeModel(sourceRow);
      index = ui.listView->currentIndex();
    }

    const auto entryKind = static_cast<RelinkingModel::EntryKind>(index.data(m_model.EntryKindRole).toInt());
    ui.useConfigPathButton->setEnabled(entryKind != RelinkingModel::OutputDirectory);
    updateConfigPathButtonAppearance();
    const QString path(index.data(m_model.UncommittedPathRole).toString());
    const int type = index.data(m_model.TypeRole).toInt();
    ui.pathVisualization->setPath(RelinkablePath(core::toFileSystemPath(path), (RelinkablePath::Type) type),
                                  /*clickable=*/true);
    ui.pathVisualization->setVisible(true);

    if (!rollback) {
      m_model.commitChanges();
    }
  }

  ui.errorLabel->setVisible(false);
}

void RelinkingDialog::pathButtonClicked(const QString& prefixPath, const QString& suffixPath, const int type) {
  assert(!prefixPath.endsWith(QChar('/')) && !prefixPath.endsWith(QChar('\\')));
  assert(!suffixPath.startsWith(QChar('/')) && !suffixPath.startsWith(QChar('\\')));

  QString replacementPath;

  if (type == RelinkablePath::File) {
    const QDir dir(QFileInfo(prefixPath).dir());
    replacementPath = QFileDialog::getOpenFileName(
        this, tr("Substitution File for %1").arg(QDir::toNativeSeparators(prefixPath)),
        dir.exists() ? dir.path() : m_projectFileDir, QString(), nullptr, QFileDialog::DontUseNativeDialog);
  } else {
    const QDir dir(prefixPath);
    replacementPath = QFileDialog::getExistingDirectory(
        this, tr("Substitution Directory for %1").arg(QDir::toNativeSeparators(prefixPath)),
        dir.exists() ? prefixPath : m_projectFileDir, QFileDialog::DontUseNativeDialog);
  }
  // So what's wrong with native dialogs? The one for directory selection won't show files
  // at all (if you ask it to, the non-native dialog will appear), which is inconvenient
  // in this situation. So, if one of them has to be non-native, the other was made
  // non-native as well, for consistency reasons.
  replacementPath = core::toQString(RelinkablePath::normalize(core::toFileSystemPath(replacementPath)));

  if (replacementPath.isEmpty()) {
    return;
  }

  if (prefixPath == replacementPath) {
    return;
  }

  const QModelIndex currentIndex(ui.listView->currentIndex());
  if (!currentIndex.isValid()) {
    return;
  }
  const auto entryKind = static_cast<RelinkingModel::EntryKind>(currentIndex.data(m_model.EntryKindRole).toInt());
  const auto pathType = static_cast<RelinkablePath::Type>(type);
  applyReplacement(prefixPath, suffixPath, replacementPath, pathType, pathType, entryKind);
}  // RelinkingDialog::pathButtonClicked

void RelinkingDialog::useConfigPath() {
  const QModelIndex index(ui.listView->currentIndex());
  if (!index.isValid()) {
    return;
  }

  const QString path(index.data(m_model.UncommittedPathRole).toString());
  const auto entryKind = static_cast<RelinkingModel::EntryKind>(index.data(m_model.EntryKindRole).toInt());
  if (entryKind == RelinkingModel::OutputDirectory) {
    return;
  }
  const auto type = static_cast<RelinkablePath::Type>(index.data(m_model.TypeRole).toInt());
  QString prefixPath;
  QString suffixPath;
  if (type == RelinkablePath::File) {
    const QFileInfo fileInfo(path);
    prefixPath = fileInfo.path();
    suffixPath = fileInfo.fileName();
  } else {
    prefixPath = path;
  }

  applyReplacement(prefixPath, suffixPath, m_projectFileDir, RelinkablePath::Dir, type, entryKind);
}

void RelinkingDialog::applyReplacement(const QString& prefixPath,
                                       const QString& suffixPath,
                                       const QString& replacementPath,
                                       const RelinkablePath::Type replacementType,
                                       const RelinkablePath::Type resultingPathType,
                                       const RelinkingModel::EntryKind entryKind) {
  if (prefixPath.isEmpty() || replacementPath.isEmpty() || prefixPath == replacementPath) {
    return;
  }

  QString newPath(replacementPath);
  newPath += QChar('/');
  newPath += suffixPath;

  const int sourceRow = selectedSourceRow();
  m_model.replacePrefix(prefixPath, replacementPath, replacementType, entryKind);
  rebuildTreeModel(sourceRow);
  updateConfigPathButtonAppearance();

  const bool hasMerges = m_model.checkForMerges();
  if (hasMerges) {
    ui.errorLabel->setText(tr("This change would merge several files into one."));
    ui.errorLabel->setVisible(true);
    ui.pathVisualization->clear();
    ui.pathVisualization->setVisible(false);
  } else {
    ui.pathVisualization->setPath(RelinkablePath(core::toFileSystemPath(newPath), resultingPathType),
                                  /*clickable=*/true);
    ui.pathVisualization->setVisible(true);
  }

  ui.useConfigPathButton->setEnabled(!hasMerges && entryKind != RelinkingModel::OutputDirectory);
  ui.undoButton->setVisible(true);
  ui.listView->update();
}

void RelinkingDialog::undoButtonClicked() {
  const int sourceRow = selectedSourceRow();
  m_model.rollbackChanges();  // Has to go before selectionChanged()
  rebuildTreeModel(sourceRow);
  selectionChanged(ui.listView->selectionModel()->selection(), QItemSelection());
}

void RelinkingDialog::commitChanges() {
  m_model.commitChanges();
  accept();
}

void RelinkingDialog::scheduleSummaryUpdate() {
  if (!m_summaryUpdateTimer.isActive()) {
    m_summaryUpdateTimer.start();
  }
}

void RelinkingDialog::updateConfigPathButtonAppearance() {
  const bool rootOutsideProject = imageRootIsOutsideProjectDirectory();
  if (ui.useConfigPathButton->property("imageRootOutsideProject").toBool() == rootOutsideProject) {
    return;
  }

  ui.useConfigPathButton->setProperty("imageRootOutsideProject", rootOutsideProject);
  if (rootOutsideProject) {
    const bool darkPalette = QApplication::palette().color(QPalette::Window).lightness() < 128;
    const QString background = darkPalette ? QStringLiteral("#755300") : QStringLiteral("#fff2cc");
    const QString foreground = darkPalette ? QStringLiteral("#fff2cc") : QStringLiteral("#694800");
    const QString hoverBackground = darkPalette ? QStringLiteral("#896300") : QStringLiteral("#ffe29a");
    ui.useConfigPathButton->setStyleSheet(
        QStringLiteral("QPushButton { background-color: %1; color: %2; border: 1px solid #b88724; }"
                       "QPushButton:hover { background-color: %3; }")
            .arg(background, foreground, hoverBackground));
    ui.useConfigPathButton->setToolTip(
        tr("The common image root is outside the folder containing this project. Use this to move it under the "
           "project folder."));
  } else {
    ui.useConfigPathButton->setStyleSheet(QString());
    ui.useConfigPathButton->setToolTip(tr("Use the folder containing this project as the selected path's base."));
  }
  ui.useConfigPathButton->style()->unpolish(ui.useConfigPathButton);
  ui.useConfigPathButton->style()->polish(ui.useConfigPathButton);
  ui.useConfigPathButton->update();
}

bool RelinkingDialog::imageRootIsOutsideProjectDirectory() const {
  QStringList imagePaths;
  QString commonRootPath;
  for (int row = 0; row < m_model.rowCount(); ++row) {
    const QModelIndex index = m_model.index(row, 0);
    const auto entryKind = static_cast<RelinkingModel::EntryKind>(index.data(m_model.EntryKindRole).toInt());
    if (entryKind == RelinkingModel::CommonImageRoot) {
      commonRootPath = index.data(m_model.UncommittedPathRole).toString();
    } else if (entryKind == RelinkingModel::ImageFile) {
      imagePaths.push_back(index.data(m_model.UncommittedPathRole).toString());
    }
  }

  if (imagePaths.isEmpty()) {
    return false;
  }
  if (commonRootPath.isEmpty()) {
    commonRootPath = RelinkingModel::findCommonImageRoot(imagePaths);
  }
  return commonRootPath.isEmpty() || !isPathWithinDirectory(commonRootPath, m_projectFileDir);
}

void RelinkingDialog::rebuildTreeModel(const int selectedRow) {
  const QSignalBlocker selectionBlocker(ui.listView->selectionModel());
  m_treeModel->clear();
  m_treeItemsByRow.fill(nullptr, m_model.rowCount());

  const int rowCount = m_model.rowCount();
  int commonRootRow = -1;
  QString commonRootPath;
  for (int row = 0; row < rowCount; ++row) {
    const QModelIndex sourceIndex = m_model.index(row, 0);
    if (sourceIndex.data(m_model.EntryKindRole).toInt() == RelinkingModel::CommonImageRoot) {
      commonRootRow = row;
      commonRootPath = sourceIndex.data(m_model.UncommittedPathRole).toString();
      break;
    }
  }

  QStandardItem* commonRootItem = nullptr;
  if (commonRootRow >= 0) {
    commonRootItem = new QStandardItem;
    copyPathData(commonRootItem, commonRootRow);
    m_treeModel->appendRow(commonRootItem);
    m_treeItemsByRow[commonRootRow] = commonRootItem;
  }

  QHash<QString, QStandardItem*> directoryNodes;
  for (int row = 0; row < rowCount; ++row) {
    const QModelIndex sourceIndex = m_model.index(row, 0);
    const auto entryKind = static_cast<RelinkingModel::EntryKind>(sourceIndex.data(m_model.EntryKindRole).toInt());
    if (entryKind == RelinkingModel::CommonImageRoot) {
      continue;
    }

    const QString path = QDir::cleanPath(sourceIndex.data(m_model.UncommittedPathRole).toString());
    const bool underCommonRoot = commonRootItem && isPathWithinDirectory(path, commonRootPath);
    QStringList components;
    QStandardItem* parentItem = nullptr;
    QString currentDirectory;

    if (underCommonRoot) {
      const QString relativePath = QDir::fromNativeSeparators(QDir(commonRootPath).relativeFilePath(path));
      components = relativePath.split(QChar('/'), Qt::SkipEmptyParts);
      parentItem = commonRootItem;
      currentDirectory = commonRootPath;
    } else {
      const QString parentDirectory = QFileInfo(path).absolutePath();
      const QString rootKey = QStringLiteral("outside:") + parentDirectory;
      parentItem = directoryNodes.value(rootKey, nullptr);
      if (!parentItem) {
        parentItem = new QStandardItem(tr("Outside common root: %1").arg(QDir::toNativeSeparators(parentDirectory)));
        parentItem->setData(true, RelinkingModel::TreeGroupRole);
        parentItem->setFlags(Qt::ItemIsEnabled);
        m_treeModel->appendRow(parentItem);
        directoryNodes.insert(rootKey, parentItem);
      }
      components.push_back(QFileInfo(path).fileName());
      currentDirectory = parentDirectory;
    }

    if (components.isEmpty()) {
      components.push_back(QFileInfo(path).fileName());
    }

    for (int componentIndex = 0; componentIndex < components.size() - 1; ++componentIndex) {
      const QString component = components.at(componentIndex);
      currentDirectory = QDir(currentDirectory).filePath(component);
      const QString key = QString::number(underCommonRoot ? 1 : 0) + QChar(':') + currentDirectory;
      QStandardItem* directoryItem = directoryNodes.value(key, nullptr);
      if (!directoryItem) {
        directoryItem = new QStandardItem(component);
        directoryItem->setData(true, RelinkingModel::TreeGroupRole);
        directoryItem->setFlags(Qt::ItemIsEnabled);
        parentItem->appendRow(directoryItem);
        directoryNodes.insert(key, directoryItem);
      }
      parentItem = directoryItem;
    }

    QStandardItem* pathItem = new QStandardItem;
    copyPathData(pathItem, row);
    parentItem->appendRow(pathItem);
    m_treeItemsByRow[row] = pathItem;
  }

  ui.listView->expandToDepth(1);
  m_treeBuilt = true;
  if (selectedRow >= 0 && selectedRow < m_treeItemsByRow.size() && m_treeItemsByRow.at(selectedRow)) {
    const QModelIndex sourceIndex = m_treeModel->indexFromItem(m_treeItemsByRow.at(selectedRow));
    const QModelIndex proxyIndex = m_sortingModel->mapFromSource(sourceIndex);
    if (proxyIndex.isValid()) {
      ui.listView->selectionModel()->setCurrentIndex(proxyIndex,
                                                     QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
    }
  }
}

void RelinkingDialog::updateTreeItems(const QModelIndex& first, const QModelIndex& last) {
  for (int row = first.row(); row <= last.row(); ++row) {
    updateTreeItem(row);
  }
}

void RelinkingDialog::updateTreeItem(const int sourceRow) {
  if (sourceRow < 0 || sourceRow >= m_treeItemsByRow.size() || !m_treeItemsByRow.at(sourceRow)) {
    return;
  }
  copyPathData(m_treeItemsByRow.at(sourceRow), sourceRow);
}

void RelinkingDialog::copyPathData(QStandardItem* target, const int sourceRow) {
  const QModelIndex sourceIndex = m_model.index(sourceRow, 0);
  const int roles[] = {Qt::DisplayRole,
                       Qt::DecorationRole,
                       Qt::ForegroundRole,
                       Qt::ToolTipRole,
                       m_model.TypeRole,
                       m_model.UncommittedPathRole,
                       m_model.UncommittedStatusRole,
                       m_model.EntryKindRole};
  for (const int role : roles) {
    target->setData(sourceIndex.data(role), role);
  }
  const auto entryKind = static_cast<RelinkingModel::EntryKind>(sourceIndex.data(m_model.EntryKindRole).toInt());
  const QString path = sourceIndex.data(m_model.UncommittedPathRole).toString();
  const QString fileName = QDir::toNativeSeparators(QFileInfo(path).fileName());
  if (entryKind == RelinkingModel::ImageFile) {
    target->setData(fileName, Qt::DisplayRole);
  } else if (entryKind == RelinkingModel::OutputDirectory) {
    target->setData(tr("%1 (output directory)").arg(fileName), Qt::DisplayRole);
  }
  target->setData(sourceRow, m_model.TreeSourceRowRole);
  target->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
}

QModelIndex RelinkingDialog::firstSelectableIndex(const QModelIndex& parent) const {
  for (int row = 0; row < m_sortingModel->rowCount(parent); ++row) {
    const QModelIndex index = m_sortingModel->index(row, 0, parent);
    if (index.data(m_model.UncommittedPathRole).isValid()) {
      return index;
    }
    const QModelIndex childIndex = firstSelectableIndex(index);
    if (childIndex.isValid()) {
      return childIndex;
    }
  }
  return {};
}

int RelinkingDialog::selectedSourceRow() const {
  const QModelIndex index = ui.listView->currentIndex();
  return index.data(m_model.TreeSourceRowRole).toInt();
}

void RelinkingDialog::updateSummary() {
  const RelinkingModel::Summary summary = m_model.summary();
  const QString successColor = RelinkingModel::foregroundColorForStatus(RelinkingModel::Exists).name();
  const QString warningColor = RelinkingModel::foregroundColorForStatus(RelinkingModel::StatusUpdateFailed).name();
  const QString errorColor = RelinkingModel::foregroundColorForStatus(RelinkingModel::Missing).name();
  const QString statusColor = summary.missingPaths > 0 || summary.failedPaths > 0 ? errorColor : successColor;

  QString outputDescription = tr("not available");
  QString outputColor = warningColor;
  if (summary.hasOutput) {
    outputDescription = summary.outputInsideRoot ? tr("inside common root") : tr("outside common root");
    outputColor = summary.outputInsideRoot ? successColor : warningColor;
    if (summary.outputStatus == RelinkingModel::Missing) {
      outputDescription += tr(", missing");
      outputColor = errorColor;
    } else if (summary.outputStatus == RelinkingModel::StatusUpdateFailed) {
      outputDescription += tr(", status unknown");
      outputColor = errorColor;
    }
  }

  const QString errors = tr("%1 missing, %2 status errors").arg(summary.missingPaths).arg(summary.failedPaths);
  const QString pending = summary.pendingPaths > 0 ? tr(" • %1 still checking").arg(summary.pendingPaths) : QString();
  const QString outsideColor = summary.imagesOutsideRoot > 0 ? warningColor : successColor;
  ui.summaryLabel->setText(tr("<span style=\"color:%1\"><b>%2</b> image(s) under common root</span> "
                              "&nbsp; <span style=\"color:%3\"><b>%4</b> outside</span><br/>"
                              "Output directory: <span style=\"color:%5\"><b>%6</b></span> &nbsp; "
                              "<span style=\"color:%7\"><b>%8</b></span>%9")
                               .arg(successColor)
                               .arg(summary.imagesInsideRoot)
                               .arg(outsideColor)
                               .arg(summary.imagesOutsideRoot)
                               .arg(outputColor)
                               .arg(outputDescription)
                               .arg(statusColor)
                               .arg(errors)
                               .arg(pending));
}