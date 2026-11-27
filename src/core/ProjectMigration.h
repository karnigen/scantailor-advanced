// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <QDomDocument>
#include <QString>
#include <filesystem>

struct ProjectMigrationReport {
  int sourceVersion = 0;
  int imagesWithDefaultTrim = 0;
  int colorSettingsWithDefaultFillOutsidePageBox = 0;
  int obliqueModesFromAutoOblique = 0;
  int obliqueModesFromDeskewMode = 0;
  int obliqueModesDisabledForMissingAttributes = 0;
  int obliqueAutoModesFromAutoOblique = 0;
  int obliqueManualModesFromAutoOblique = 0;
  int obliqueAutoModesFromDeskewMode = 0;
  int obliqueManualModesFromDeskewMode = 0;
  int pathsWrittenRelative = 0;
};

class ProjectMigration {
 public:
  static ProjectMigrationReport analyze(const QDomDocument& doc, const std::filesystem::path& projectFilePath);

  static bool createBackupForFormat3(const std::filesystem::path& projectFilePath,
                                     std::filesystem::path* backupFilePath,
                                     QString* errorMessage = nullptr);
};
