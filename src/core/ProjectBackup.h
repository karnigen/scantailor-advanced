// SPDX-FileCopyrightText: 2026 ScanTailor Advanced contributors
// SPDX-License-Identifier: GPL-3.0-only

#ifndef SCANTAILOR_CORE_PROJECTBACKUP_H_
#define SCANTAILOR_CORE_PROJECTBACKUP_H_

#include <QString>
#include <filesystem>

bool createProjectBackup(const std::filesystem::path& projectFilePath,
                         int backupCount,
                         QString* errorMessage = nullptr);

#endif  // SCANTAILOR_CORE_PROJECTBACKUP_H_
