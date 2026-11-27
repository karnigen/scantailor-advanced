// SPDX-FileCopyrightText: 2026 ScanTailor Advanced contributors
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <QString>
#include <filesystem>

bool createProjectBackup(const std::filesystem::path& projectFilePath,
                         int backupCount,
                         QString* errorMessage = nullptr);
