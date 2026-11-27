// SPDX-FileCopyrightText: 2026 karnigen
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <QString>

class CompletionSound {
 public:
  static bool play(const QString& command = QString());
};
