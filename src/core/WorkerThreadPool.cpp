// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "WorkerThreadPool.h"

#include <QCoreApplication>
#include <QThreadPool>
#include <exception>
#include <utility>

#include "OutOfMemoryHandler.h"

class WorkerThreadPool::TaskResultEvent : public QEvent {
 public:
  TaskResultEvent(BackgroundTaskPtr task, FilterResultPtr result, QString errorMessage)
      : QEvent(User), m_task(std::move(task)), m_result(std::move(result)), m_errorMessage(std::move(errorMessage)) {}

  const BackgroundTaskPtr& task() const { return m_task; }

  const FilterResultPtr& result() const { return m_result; }

  const QString& errorMessage() const { return m_errorMessage; }

 private:
  BackgroundTaskPtr m_task;
  FilterResultPtr m_result;
  QString m_errorMessage;
};


WorkerThreadPool::WorkerThreadPool(QObject* parent) : QObject(parent), m_pool(new QThreadPool(this)) {
  updateNumberOfThreads();
}

WorkerThreadPool::~WorkerThreadPool() = default;

void WorkerThreadPool::shutdown() {
  m_pool->waitForDone();
}

bool WorkerThreadPool::hasSpareCapacity() const {
  return m_pool->activeThreadCount() < m_pool->maxThreadCount();
}

void WorkerThreadPool::submitTask(const BackgroundTaskPtr& task) {
  class Runnable : public QRunnable {
   public:
    Runnable(WorkerThreadPool& owner, BackgroundTaskPtr task) : m_owner(owner), m_task(std::move(task)) {
      setAutoDelete(true);
    }

    void run() override {
      if (m_task->isCancelled()) {
        return;
      }

      const auto makeExceptionResult = [this](const QString& message) {
        try {
          return m_task->exceptionResult(message);
        } catch (const std::bad_alloc&) {
          OutOfMemoryHandler::instance().handleOutOfMemorySituation();
        } catch (const std::exception& e) {
          qWarning("Failed to create a background task error result: %s", e.what());
        } catch (...) {
          qWarning("Failed to create a background task error result: unknown exception.");
        }
        return FilterResultPtr();
      };

      FilterResultPtr result;
      QString errorMessage;
      bool taskFailed = false;
      try {
        result = (*m_task)();
      } catch (const BackgroundTask::CancelledException&) {
        taskFailed = true;
        errorMessage = QStringLiteral("Background task was cancelled unexpectedly.");
      } catch (const std::bad_alloc&) {
        OutOfMemoryHandler::instance().handleOutOfMemorySituation();
        taskFailed = true;
        result = makeExceptionResult(QString());
      } catch (const std::exception& e) {
        taskFailed = true;
        qWarning("Background task failed: %s", e.what());
        try {
          errorMessage = QString::fromUtf8(e.what());
        } catch (const std::bad_alloc&) {
          OutOfMemoryHandler::instance().handleOutOfMemorySituation();
        } catch (const std::exception& error) {
          qWarning("Failed to capture a background task error message: %s", error.what());
        } catch (...) {
          qWarning("Failed to capture a background task error message: unknown exception.");
        }
        result = makeExceptionResult(errorMessage);
      } catch (...) {
        taskFailed = true;
        qWarning("Background task failed with an unknown exception.");
        result = makeExceptionResult(QString());
      }

      if (result || taskFailed) {
        try {
          QCoreApplication::postEvent(&m_owner, new TaskResultEvent(m_task, result, errorMessage));
        } catch (const std::bad_alloc&) {
          OutOfMemoryHandler::instance().handleOutOfMemorySituation();
        } catch (const std::exception& e) {
          qWarning("Failed to post a background task result: %s", e.what());
        } catch (...) {
          qWarning("Failed to post a background task result: unknown exception.");
        }
      }
    }

   private:
    WorkerThreadPool& m_owner;
    BackgroundTaskPtr m_task;
  };


  updateNumberOfThreads();
  m_pool->start(new Runnable(*this, task));
}  // WorkerThreadPool::submitTask

void WorkerThreadPool::customEvent(QEvent* event) {
  if (auto* evt = dynamic_cast<TaskResultEvent*>(event)) {
    emit taskResult(evt->task(), evt->result(), evt->errorMessage());
  }
}

void WorkerThreadPool::updateNumberOfThreads() {
  int maxThreads = QThread::idealThreadCount();
  // Restricting num of processors for 32-bit due to
  // address space constraints.
  if (sizeof(void*) <= 4) {
    maxThreads = std::min(maxThreads, 2);
  }

  int numThreads = m_settings.value("settings/batch_processing_threads", maxThreads).toInt();
  numThreads = std::min(numThreads, maxThreads);
  m_pool->setMaxThreadCount(numThreads);
}
