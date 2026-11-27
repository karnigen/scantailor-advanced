// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <QAtomicInt>
#include <QString>
#include <exception>
#include <memory>

#include "AbstractCommand.h"
#include "FilterResult.h"
#include "TaskStatus.h"

class BackgroundTask : public AbstractCommand<FilterResultPtr>, public TaskStatus {
 public:
  enum Type { INTERACTIVE, BATCH };

  class CancelledException : public std::exception {
   public:
    const char* what() const noexcept override;
  };


  explicit BackgroundTask(Type type) : m_type(type) {}

  Type type() const { return m_type; }

  void cancel() override { m_cancelFlag.storeRelaxed(1); }

  bool isCancelled() const override { return m_cancelFlag.loadRelaxed() != 0; }

  /**
   * \brief If cancelled, throws CancelledException.
   */
  void throwIfCancelled() const override;

  virtual FilterResultPtr exceptionResult(const QString& message) = 0;

 private:
  QAtomicInt m_cancelFlag;
  const Type m_type;
};


using BackgroundTaskPtr = std::shared_ptr<BackgroundTask>;
