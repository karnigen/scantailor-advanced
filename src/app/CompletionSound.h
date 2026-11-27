// SPDX-FileCopyrightText: 2026 karnigen
// SPDX-License-Identifier: GPL-3.0-only

#ifndef SCANTAILOR_APP_COMPLETIONSOUND_H_
#define SCANTAILOR_APP_COMPLETIONSOUND_H_

#include <QString>

class CompletionSound {
 public:
  static bool play(const QString& command = QString());
};

#endif  // SCANTAILOR_APP_COMPLETIONSOUND_H_
