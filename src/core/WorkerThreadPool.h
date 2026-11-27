// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#ifndef SCANTAILOR_CORE_WORKERTHREADPOOL_H_
#define SCANTAILOR_CORE_WORKERTHREADPOOL_H_

#include <QObject>
#include <memory>

#include "BackgroundTask.h"
#include "FilterResult.h"
#include "SettingsStore.h"

class QThreadPool;

class WorkerThreadPool : public QObject {
  Q_OBJECT
 public:
  explicit WorkerThreadPool(QObject* parent = nullptr);

  ~WorkerThreadPool() override;

  /**
   * \brief Waits for pending jobs to finish and stop the thread.
   *
   * The destructor also performs these tasks, so this method is only
   * useful to prematuraly stop task processing.
   */
  void shutdown();

  bool hasSpareCapacity() const;

  bool isIdle() const;

  void submitTask(const BackgroundTaskPtr& task);

 signals:

  void taskResult(const BackgroundTaskPtr& task, const FilterResultPtr& result, const QString& errorMessage);

  void idle();

 private:
  class TaskResultEvent;

  void customEvent(QEvent* event) override;

  void updateNumberOfThreads();

  QThreadPool* m_pool;
  int m_pendingTasks = 0;
  SettingsStore m_settings;
};


#endif  // ifndef SCANTAILOR_CORE_WORKERTHREADPOOL_H_
