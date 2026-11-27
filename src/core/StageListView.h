// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <QPixmap>
#include <QTableView>
#include <memory>
#include <vector>

class StageSequence;

class StageListView : public QTableView {
  Q_OBJECT
 public:
  explicit StageListView(QWidget* parent);

  ~StageListView() override;

  void setStages(const std::shared_ptr<StageSequence>& stages);

  QSize sizeHint() const override { return m_sizeHint; }

 signals:

  void launchBatchProcessing();

 public slots:

  void setBatchProcessingPossible(bool possible);

  void setBatchProcessingInProgress(bool inProgress);

 protected slots:

  void selectionChanged(const QItemSelection& selected, const QItemSelection& deselected) override;

 private slots:

  void ensureSelectedRowVisible();

 protected:
  void timerEvent(QTimerEvent* event) override;

 private:
  class Model;
  class LeftColDelegate;

  class RightColDelegate;

  void removeLaunchButton(int row);

  void placeLaunchButton(int row);

  void initiateBatchAnimationFrameRendering();

  void createBatchAnimationSequence(int squareSide);

  void updateRowSpans();

  int selectedRow() const;

  QSize m_sizeHint;
  Model* m_model;
  LeftColDelegate* m_firstColDelegate;
  RightColDelegate* m_secondColDelegate;
  QWidget* m_launchBtn;
  std::vector<QPixmap> m_batchAnimationPixmaps;
  int m_curBatchAnimationFrame;
  int m_timerId;
  bool m_batchProcessingPossible;
  bool m_batchProcessingInProgress;
};
