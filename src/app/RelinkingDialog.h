// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_APP_RELINKINGDIALOG_H_
#define SCANTAILOR_APP_RELINKINGDIALOG_H_

#include <QDialog>
#include <QStandardItemModel>
#include <QTimer>
#include <QVector>
#include <memory>

#include "AbstractRelinker.h"
#include "RelinkablePath.h"
#include "RelinkingModel.h"
#include "ui_RelinkingDialog.h"

class RelinkingSortingModel;
class QAbstractButton;
class QItemSelection;
class QStandardItem;
class QString;

class RelinkingDialog : public QDialog {
  Q_OBJECT
 public:
  explicit RelinkingDialog(const QString& projectFilePath,
                           QWidget* parent = nullptr,
                           const QString& initialDirectory = QString());

  ProxyFunction<RelinkingModel&, void, const RelinkablePath&> pathCollector() {
    return ProxyFunction<RelinkingModel&, void, const RelinkablePath&>(m_model);
  }

  void addCommonImageRoot() { m_model.addCommonImageRoot(); }

  void addOutputPath(const RelinkablePath& path) { m_model.addOutputPath(path); }

  /**
   * This method guarantees that
   * \code
   * dialog->relinker().release() == dialog->relinker().release()
   * \endcode
   * will hold true for the lifetime of the dialog.
   * This allows you to take the relinker right after construction
   * and then use it when accepted() signal is emitted.
   */
  std::shared_ptr<AbstractRelinker> relinker() const { return m_model.relinker(); }

  void selectFirstPath();

 private slots:

  void selectionChanged(const QItemSelection& selected, const QItemSelection& deselected);

  /** \p type is either RelinkablePath::File or RelinkablePath::Dir */
  void pathButtonClicked(const QString& prefixPath, const QString& suffixPath, int type);

  void useConfigPath();

  void undoButtonClicked();

  void commitChanges();

 private:
  void updateSummary();

  void updateConfigPathButtonAppearance();

  bool imageRootIsOutsideProjectDirectory() const;

  void scheduleSummaryUpdate();

  void rebuildTreeModel(int selectedSourceRow = -1);

  void updateTreeItems(const QModelIndex& first, const QModelIndex& last);

  void updateTreeItem(int sourceRow);

  void copyPathData(QStandardItem* target, int sourceRow);

  QModelIndex firstSelectableIndex(const QModelIndex& parent = QModelIndex()) const;

  int selectedSourceRow() const;

  void applyReplacement(const QString& prefixPath,
                        const QString& suffixPath,
                        const QString& replacementPath,
                        RelinkablePath::Type replacementType,
                        RelinkablePath::Type resultingPathType,
                        RelinkingModel::EntryKind entryKind);

  Ui::RelinkingDialog ui;
  RelinkingModel m_model;
  QStandardItemModel* m_treeModel;
  RelinkingSortingModel* m_sortingModel;
  QVector<QStandardItem*> m_treeItemsByRow;
  QTimer m_summaryUpdateTimer;
  QString m_projectFileDir;
  bool m_treeBuilt = false;
};


#endif  // ifndef SCANTAILOR_APP_RELINKINGDIALOG_H_
