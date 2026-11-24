// Copyright (C) 2026
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_APP_COMPLETIONSOUND_H_
#define SCANTAILOR_APP_COMPLETIONSOUND_H_

#include <QString>

class CompletionSound {
 public:
  static bool play(const QString& command = QString());
};

#endif  // SCANTAILOR_APP_COMPLETIONSOUND_H_
