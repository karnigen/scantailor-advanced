// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include "TaskStatus.h"

class NullTaskStatus : public TaskStatus {
  void cancel() override {}

  bool isCancelled() const override { return false; }

  void throwIfCancelled() const override {}
};
