// SPDX-FileCopyrightText: 2026 project contributors
// SPDX-License-Identifier: GPL-3.0-only

#include "ProjectBackup.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSaveFile>

#include "PathUtils.h"

bool createProjectBackup(const std::filesystem::path& projectFilePath, const int backupCount, QString* errorMessage) {
  if (errorMessage) {
    errorMessage->clear();
  }
  if (backupCount <= 0) {
    return true;
  }

  const QString qtProjectFilePath(core::toQString(projectFilePath));
  QFile source(qtProjectFilePath);
  if (!source.open(QIODevice::ReadOnly)) {
    if (errorMessage) {
      *errorMessage = source.errorString();
    }
    return false;
  }

  std::filesystem::path backupDirectoryPath(projectFilePath);
  backupDirectoryPath += ".backups";
  const QString qtBackupDirectoryPath(core::toQString(backupDirectoryPath));
  if (!QDir().mkpath(qtBackupDirectoryPath)) {
    if (errorMessage) {
      *errorMessage = QStringLiteral("Unable to create the project backup directory: %1").arg(qtBackupDirectoryPath);
    }
    return false;
  }

  const QDir backupDirectory(qtBackupDirectoryPath);
  const QString timestamp = QDateTime::currentDateTimeUtc().toString(QStringLiteral("yyyyMMdd-HHmmss-zzz"));
  std::filesystem::path backupPath;
  for (int suffix = 0; suffix < 1000; ++suffix) {
    const QString suffixText = suffix == 0 ? QString() : QStringLiteral("-%1").arg(suffix, 3, 10, QLatin1Char('0'));
    const QString fileName = timestamp + suffixText + QStringLiteral(".ScanTailor");
    backupPath = backupDirectoryPath / core::toFileSystemPath(fileName);
    if (!QFileInfo::exists(core::toQString(backupPath))) {
      break;
    }
    backupPath.clear();
  }
  if (backupPath.empty()) {
    if (errorMessage) {
      *errorMessage = QStringLiteral("Unable to choose a unique project backup filename.");
    }
    return false;
  }

  const QString qtBackupPath(core::toQString(backupPath));
  QSaveFile backup(qtBackupPath);
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
    const std::filesystem::path oldBackupPath = backupDirectoryPath / core::toFileSystemPath(backups.at(index));
    const QString qtOldBackupPath(core::toQString(oldBackupPath));
    if (!QFile::remove(qtOldBackupPath)) {
      if (errorMessage) {
        *errorMessage = QStringLiteral("Unable to remove an expired project backup: %1").arg(qtOldBackupPath);
      }
      return false;
    }
  }
  return true;
}
