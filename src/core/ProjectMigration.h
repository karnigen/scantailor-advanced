// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_CORE_PROJECTMIGRATION_H_
#define SCANTAILOR_CORE_PROJECTMIGRATION_H_

#include <QDomDocument>
#include <QString>

struct ProjectMigrationReport {
  int sourceVersion = 0;
  int imagesWithDefaultTrim = 0;
  int colorSettingsWithDefaultFillOutsidePageBox = 0;
  int obliqueModesFromAutoOblique = 0;
  int obliqueModesFromDeskewMode = 0;
  int obliqueAutoModesFromAutoOblique = 0;
  int obliqueManualModesFromAutoOblique = 0;
  int obliqueAutoModesFromDeskewMode = 0;
  int obliqueManualModesFromDeskewMode = 0;
  int pathsWrittenRelative = 0;
};

class ProjectMigration {
 public:
  static ProjectMigrationReport analyze(const QDomDocument& doc, const QString& projectFilePath);

  static bool createBackupForFormat3(const QString& projectFilePath,
                                     QString* backupFilePath,
                                     QString* errorMessage = nullptr);
};

#endif  // SCANTAILOR_CORE_PROJECTMIGRATION_H_
