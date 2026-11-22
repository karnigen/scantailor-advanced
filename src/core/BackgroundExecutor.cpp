// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

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
  bool m_threadStarted;
  std::atomic_bool m_shuttingDown{false};
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

BackgroundExecutor::Impl::Impl(BackgroundExecutor& owner)
    : m_owner(owner), m_dispatcher(*this), m_threadStarted(false) {
  m_dispatcher.moveToThread(this);
}

BackgroundExecutor::Impl::~Impl() {
  m_shuttingDown.store(true);
  exit();
  wait();
}

void BackgroundExecutor::Impl::enqueueTask(const TaskPtr& task) {
  QCoreApplication::postEvent(&m_dispatcher, new TaskEvent(task));
  if (!m_threadStarted) {
    start();
    m_threadStarted = true;
  }
}

void BackgroundExecutor::Impl::run() {
  while (!m_shuttingDown.load()) {
    try {
      exec();
      return;
    } catch (const std::bad_alloc&) {
      OutOfMemoryHandler::instance().handleOutOfMemorySituation();
    } catch (const std::exception& e) {
      qCritical("Background executor event loop failed: %s", e.what());
    } catch (...) {
      qCritical("Background executor event loop failed with an unknown exception.");
    }
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
