// Copyright (C) 2026 ScanTailor Advanced contributors
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_CORE_PROJECTBACKUP_H_
#define SCANTAILOR_CORE_PROJECTBACKUP_H_

#include <QString>

bool createProjectBackup(const QString& projectFilePath, int backupCount, QString* errorMessage = nullptr);

#endif  // SCANTAILOR_CORE_PROJECTBACKUP_H_
