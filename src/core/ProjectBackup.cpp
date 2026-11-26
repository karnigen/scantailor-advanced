// Copyright (C) 2026 ScanTailor Advanced contributors
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "ProjectBackup.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSaveFile>

bool createProjectBackup(const QString& projectFilePath, const int backupCount, QString* errorMessage) {
  if (errorMessage) {
    errorMessage->clear();
  }
  if (backupCount <= 0) {
    return true;
  }

  QFile source(projectFilePath);
  if (!source.open(QIODevice::ReadOnly)) {
    if (errorMessage) {
      *errorMessage = source.errorString();
    }
    return false;
  }

  const QString backupDirectoryPath = projectFilePath + QStringLiteral(".backups");
  if (!QDir().mkpath(backupDirectoryPath)) {
    if (errorMessage) {
      *errorMessage = QStringLiteral("Unable to create the project backup directory: %1").arg(backupDirectoryPath);
    }
    return false;
  }

  const QDir backupDirectory(backupDirectoryPath);
  const QString timestamp = QDateTime::currentDateTimeUtc().toString(QStringLiteral("yyyyMMdd-HHmmss-zzz"));
  QString backupPath;
  for (int suffix = 0; suffix < 1000; ++suffix) {
    const QString suffixText = suffix == 0 ? QString() : QStringLiteral("-%1").arg(suffix, 3, 10, QLatin1Char('0'));
    const QString fileName = timestamp + suffixText + QStringLiteral(".ScanTailor");
    backupPath = backupDirectory.filePath(fileName);
    if (!QFileInfo::exists(backupPath)) {
      break;
    }
    backupPath.clear();
  }
  if (backupPath.isEmpty()) {
    if (errorMessage) {
      *errorMessage = QStringLiteral("Unable to choose a unique project backup filename.");
    }
    return false;
  }

  QSaveFile backup(backupPath);
  if (!backup.open(QIODevice::WriteOnly)) {
    if (errorMessage) {
      *errorMessage = backup.errorString();
    }
    return false;
  }

  while (!source.atEnd()) {
    const QByteArray contents = source.read(1024 * 1024);
    if (contents.isEmpty() && source.error() != QFileDevice::NoError) {
      backup.cancelWriting();
      if (errorMessage) {
        *errorMessage = source.errorString();
      }
      return false;
    }
    if (backup.write(contents) != contents.size()) {
      backup.cancelWriting();
      if (errorMessage) {
        *errorMessage = backup.errorString();
      }
      return false;
    }
  }
  if (source.error() != QFileDevice::NoError) {
    backup.cancelWriting();
    if (errorMessage) {
      *errorMessage = source.errorString();
    }
    return false;
  }

  if (!backup.commit()) {
    if (errorMessage) {
      *errorMessage = backup.errorString();
    }
    return false;
  }

  const QStringList backups = backupDirectory.entryList({QStringLiteral("*.ScanTailor")}, QDir::Files, QDir::Name);
  for (int index = 0; index < backups.size() - backupCount; ++index) {
    const QString oldBackupPath = backupDirectory.filePath(backups.at(index));
    if (!QFile::remove(oldBackupPath)) {
      if (errorMessage) {
        *errorMessage = QStringLiteral("Unable to remove an expired project backup: %1").arg(oldBackupPath);
      }
      return false;
    }
  }
  return true;
}
