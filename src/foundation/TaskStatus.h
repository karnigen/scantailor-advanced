// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#ifndef SCANTAILOR_FOUNDATION_TASKSTATUS_H_
#define SCANTAILOR_FOUNDATION_TASKSTATUS_H_

class TaskStatus {
 public:
  virtual ~TaskStatus() = default;

  virtual void cancel() = 0;

  virtual bool isCancelled() const = 0;

  virtual void throwIfCancelled() const = 0;
};


#endif
