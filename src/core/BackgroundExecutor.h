// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#ifndef SCANTAILOR_CORE_BACKGROUNDEXECUTOR_H_
#define SCANTAILOR_CORE_BACKGROUNDEXECUTOR_H_

#include <memory>
#include <utility>

#include <QString>
#include "AbstractCommand.h"
#include "NonCopyable.h"
#include "PayloadEvent.h"

class BackgroundExecutor {
  DECLARE_NON_COPYABLE(BackgroundExecutor)

 public:
  using TaskResultPtr = std::shared_ptr<AbstractCommand<void>>;

  class Task : public AbstractCommand<TaskResultPtr> {
   public:
    virtual void handleException(const QString& message) = 0;
  };

  using TaskPtr = std::shared_ptr<Task>;

  BackgroundExecutor();

  /**
   * \brief Waits for background tasks to finish, then destroys the object.
   */
  ~BackgroundExecutor();

  /**
   * \brief Waits for pending jobs to finish and stop the background thread.
   *
   * The destructor also performs these tasks, so this method is only
   * useful to prematuraly stop task processing.  After shutdown, any
   * attempts to enqueue a task will be silently ignored.
   */
  void shutdown();

  /**
   * \brief Enqueue a task for execution in a background thread.
   *
   * A task is a functor to be executed in a background thread.
   * That functor may optionally return another one, that is
   * to be executed in the thread where this BackgroundExecutor
   * object was constructed.
   */
  void enqueueTask(const TaskPtr& task);

 private:
  class Impl;
  class Dispatcher;

  using TaskEvent = PayloadEvent<TaskPtr>;
  using ResultEvent = PayloadEvent<std::pair<TaskPtr, TaskResultPtr>>;
  using FailureEvent = PayloadEvent<std::pair<TaskPtr, QString>>;

  std::unique_ptr<Impl> m_impl;
};


#endif  // ifndef SCANTAILOR_CORE_BACKGROUNDEXECUTOR_H_
