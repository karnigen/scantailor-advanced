// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "RelinkingDialog.h"

#include <core/IconProvider.h>

#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QItemSelectionModel>
#include <cassert>

#include "RelinkingSortingModel.h"

RelinkingDialog::RelinkingDialog(const QString& projectFilePath, QWidget* parent, const QString& initialDirectory)
    : QDialog(parent),
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
  m_sortingModel->setSourceModel(&m_model);
  ui.listView->setModel(m_sortingModel);
  ui.listView->setTextElideMode(Qt::ElideMiddle);
  ui.errorLabel->setVisible(false);
  ui.undoButton->setVisible(false);
  ui.useConfigPathButton->setEnabled(false);
  ui.summaryLabel->setTextFormat(Qt::RichText);
  updateSummary();

  connect(ui.listView->selectionModel(), SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)),
          SLOT(selectionChanged(const QItemSelection&, const QItemSelection&)));

  connect(&m_model, &QAbstractItemModel::dataChanged, this, [this]() { updateSummary(); });
  connect(&m_model, &QAbstractItemModel::rowsInserted, this, [this]() { updateSummary(); });

  connect(ui.pathVisualization, SIGNAL(clicked(const QString&, const QString&, int)),
          SLOT(pathButtonClicked(const QString&, const QString&, int)));

  connect(ui.undoButton, SIGNAL(clicked()), SLOT(undoButtonClicked()));
  connect(ui.useConfigPathButton, SIGNAL(clicked()), SLOT(useConfigPath()));

  disconnect(ui.buttonBox, SIGNAL(accepted()));
  connect(ui.buttonBox, SIGNAL(accepted()), SLOT(commitChanges()));
}

void RelinkingDialog::selectFirstPath() {
  const QModelIndex firstIndex(m_sortingModel->index(0, 0));
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

    const QModelIndex index(selected.front().topLeft());
    const auto entryKind = static_cast<RelinkingModel::EntryKind>(index.data(m_model.EntryKindRole).toInt());
    ui.useConfigPathButton->setEnabled(entryKind != RelinkingModel::OutputDirectory);
    const QString path(index.data(m_model.UncommittedPathRole).toString());
    const int type = index.data(m_model.TypeRole).toInt();
    ui.pathVisualization->setPath(RelinkablePath(path, (RelinkablePath::Type) type), /*clickable=*/true);
    ui.pathVisualization->setVisible(true);

    if (ui.errorLabel->isVisible()) {
      m_model.rollbackChanges();
    } else {
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
  replacementPath = RelinkablePath::normalize(replacementPath);

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

  m_model.replacePrefix(prefixPath, replacementPath, replacementType, entryKind);

  const bool hasMerges = m_model.checkForMerges();
  if (hasMerges) {
    ui.errorLabel->setText(tr("This change would merge several files into one."));
    ui.errorLabel->setVisible(true);
    ui.pathVisualization->clear();
    ui.pathVisualization->setVisible(false);
  } else {
    ui.pathVisualization->setPath(RelinkablePath(newPath, resultingPathType), /*clickable=*/true);
    ui.pathVisualization->setVisible(true);
  }

  ui.useConfigPathButton->setEnabled(!hasMerges && entryKind != RelinkingModel::OutputDirectory);
  ui.undoButton->setVisible(true);
  ui.listView->update();
}

void RelinkingDialog::undoButtonClicked() {
  m_model.rollbackChanges();  // Has to go before selectionChanged()
  selectionChanged(ui.listView->selectionModel()->selection(), QItemSelection());
}

void RelinkingDialog::commitChanges() {
  m_model.commitChanges();
  accept();
}

void RelinkingDialog::updateSummary() {
  const RelinkingModel::Summary summary = m_model.summary();
  const QString successColor = QStringLiteral("#1b5e20");
  const QString warningColor = QStringLiteral("#6d4c00");
  const QString errorColor = QStringLiteral("#b71c1c");
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