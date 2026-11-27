// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <ctime>

class PerformanceTimer {
 public:
  PerformanceTimer() : m_start(clock()) {}

  void print(const char* prefix = "");

 private:
  const clock_t m_start;
};
