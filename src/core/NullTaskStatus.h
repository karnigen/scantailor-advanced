// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#ifndef SCANTAILOR_CORE_NULLTASKSTATUS_H_
#define SCANTAILOR_CORE_NULLTASKSTATUS_H_

#include "TaskStatus.h"

class NullTaskStatus : public TaskStatus {
  void cancel() override {}

  bool isCancelled() const override { return false; }

  void throwIfCancelled() const override {}
};

#endif  // SCANTAILOR_CORE_NULLTASKSTATUS_H_
