// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#include "BackgroundExecutor.h"

#include <QCoreApplication>
#include <QThread>
#include <atomic>
#include <exception>
#include <utility>

#include "OutOfMemoryHandler.h"

namespace {

void executeTaskException(const BackgroundExecutor::TaskPtr& task, const QString& message) {
  try {
    task->handleException(message);
  } catch (const std::bad_alloc&) {
    OutOfMemoryHandler::instance().handleOutOfMemorySituation();
  } catch (const std::exception& e) {
    qWarning("Background task exception handler failed: %s", e.what());
  } catch (...) {
    qWarning("Background task exception handler failed with an unknown exception.");
  }
}

}  // namespace

class BackgroundExecutor::Dispatcher : public QObject {
 public:
  explicit Dispatcher(Impl& owner);

 protected:
  void customEvent(QEvent* event) override;

 private:
  Impl& m_owner;
};


class BackgroundExecutor::Impl : public QThread {
 public:
  explicit Impl(BackgroundExecutor& owner);

  ~Impl() override;

  void enqueueTask(const TaskPtr& task);

 protected:
  void run() override;

  void customEvent(QEvent* event) override;

 private:
  BackgroundExecutor& m_owner;
  Dispatcher m_dispatcher;
  std::atomic_bool m_threadStarted{false};
  std::atomic_bool m_shuttingDown{false};
  std::atomic_int m_threadRestartCount{0};
};


/*============================ BackgroundExecutor ==========================*/

BackgroundExecutor::BackgroundExecutor() : m_impl(std::make_unique<Impl>(*this)) {}

BackgroundExecutor::~BackgroundExecutor() = default;

void BackgroundExecutor::shutdown() {
  m_impl.reset();
}

void BackgroundExecutor::enqueueTask(const TaskPtr& task) {
  if (m_impl) {
    m_impl->enqueueTask(task);
  }
}

/*===================== BackgroundExecutor::Dispatcher =====================*/

BackgroundExecutor::Dispatcher::Dispatcher(Impl& owner) : m_owner(owner) {}

void BackgroundExecutor::Dispatcher::customEvent(QEvent* event) {
  try {
    auto* evt = dynamic_cast<TaskEvent*>(event);
    if (!evt) {
      qWarning("Background executor received an unexpected task event.");
      return;
    }

    const TaskPtr task = evt->payload();
    if (!task) {
      qWarning("Background executor received an empty task.");
      return;
    }

    TaskResultPtr result;
    try {
      result = (*task)();
    } catch (const std::bad_alloc&) {
      OutOfMemoryHandler::instance().handleOutOfMemorySituation();
      QCoreApplication::postEvent(&m_owner, new FailureEvent(std::make_pair(task, QString())));
      return;
    } catch (const std::exception& e) {
      qWarning("Background task failed: %s", e.what());
      QCoreApplication::postEvent(&m_owner, new FailureEvent(std::make_pair(task, QString::fromUtf8(e.what()))));
      return;
    } catch (...) {
      qWarning("Background task failed with an unknown exception.");
      QCoreApplication::postEvent(&m_owner, new FailureEvent(std::make_pair(task, QString())));
      return;
    }
    if (result) {
      QCoreApplication::postEvent(&m_owner, new ResultEvent(std::make_pair(task, result)));
    }
  } catch (const std::bad_alloc&) {
    OutOfMemoryHandler::instance().handleOutOfMemorySituation();
  } catch (const std::exception& e) {
    qWarning("Background executor failed: %s", e.what());
  } catch (...) {
    qWarning("Background executor failed with an unknown exception.");
  }
}

/*======================= BackgroundExecutor::Impl =========================*/

BackgroundExecutor::Impl::Impl(BackgroundExecutor& owner) : m_owner(owner), m_dispatcher(*this) {
  m_dispatcher.moveToThread(this);
  connect(
      this, &QThread::finished, this,
      [this]() {
        m_threadStarted.store(false);
        if (m_shuttingDown.load()) {
          return;
        }

        constexpr int maxAutomaticRestarts = 3;
        const int restartCount = m_threadRestartCount.fetch_add(1) + 1;
        if (restartCount > maxAutomaticRestarts) {
          qCritical("Background executor is unavailable after repeated event loop failures; a later task can retry.");
          return;
        }

        qWarning("Background executor thread stopped unexpectedly; restarting (%d/%d).", restartCount,
                 maxAutomaticRestarts);
        if (!m_threadStarted.exchange(true)) {
          start();
        }
      },
      Qt::QueuedConnection);
}

BackgroundExecutor::Impl::~Impl() {
  m_shuttingDown.store(true);
  exit();
  wait();
}

void BackgroundExecutor::Impl::enqueueTask(const TaskPtr& task) {
  QCoreApplication::postEvent(&m_dispatcher, new TaskEvent(task));
  if (!m_threadStarted.exchange(true)) {
    m_threadRestartCount.store(0);
    start();
  }
}

void BackgroundExecutor::Impl::run() {
  constexpr int maxEventLoopAttempts = 3;
  int consecutiveFailures = 0;
  while (!m_shuttingDown.load() && consecutiveFailures < maxEventLoopAttempts) {
    try {
      exec();
      return;
    } catch (const std::bad_alloc&) {
      ++consecutiveFailures;
      try {
        OutOfMemoryHandler::instance().handleOutOfMemorySituation();
      } catch (const std::exception& e) {
        qCritical("Out-of-memory handler failed: %s", e.what());
      } catch (...) {
        qCritical("Out-of-memory handler failed with an unknown exception.");
      }
      qCritical("Background executor event loop failed; restarting (%d).", consecutiveFailures);
    } catch (const std::exception& e) {
      ++consecutiveFailures;
      qCritical("Background executor event loop failed: %s; restarting (%d).", e.what(), consecutiveFailures);
    } catch (...) {
      ++consecutiveFailures;
      qCritical("Background executor event loop failed with an unknown exception; restarting (%d).",
                consecutiveFailures);
    }
  }
  if (!m_shuttingDown.load() && consecutiveFailures >= maxEventLoopAttempts) {
    qCritical("Background executor event loop failed after %d attempts.", consecutiveFailures);
  }
}

void BackgroundExecutor::Impl::customEvent(QEvent* event) {
  if (auto* failureEvent = dynamic_cast<FailureEvent*>(event)) {
    const TaskPtr& task = failureEvent->payload().first;
    if (!task) {
      qWarning("Background executor received an empty failure event.");
      return;
    }
    executeTaskException(task, failureEvent->payload().second);
    return;
  }

  auto* evt = dynamic_cast<ResultEvent*>(event);
  if (!evt) {
    qWarning("Background executor received an unexpected result event.");
    return;
  }

  const TaskPtr& task = evt->payload().first;
  const TaskResultPtr& result = evt->payload().second;
  if (!task || !result) {
    qWarning("Background executor received an incomplete result event.");
    return;
  }

  try {
    (*result)();
  } catch (const std::bad_alloc&) {
    OutOfMemoryHandler::instance().handleOutOfMemorySituation();
    executeTaskException(task, QString());
  } catch (const std::exception& e) {
    qWarning("Background task result failed: %s", e.what());
    executeTaskException(task, QString::fromUtf8(e.what()));
  } catch (...) {
    qWarning("Background task result failed with an unknown exception.");
    executeTaskException(task, QString());
  }
}
