// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#include "BackgroundTask.h"

const char* BackgroundTask::CancelledException::what() const noexcept {
  return "BackgroundTask cancelled";
}

void BackgroundTask::throwIfCancelled() const {
  if (isCancelled()) {
    throw CancelledException();
  }
}
