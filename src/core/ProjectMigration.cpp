// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#include "ProjectMigration.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSet>

#include <string>

#include "PathUtils.h"

namespace {

bool willBeWrittenRelative(const QString& path, const QDir& projectDir) {
  if (path.isEmpty()) {
    return false;
  }

  const QString absolutePath = QDir::isAbsolutePath(path) ? path : projectDir.absoluteFilePath(path);
  const QString relativePath = projectDir.relativeFilePath(absolutePath);
  return !QDir::isAbsolutePath(relativePath) && relativePath != path;
}

std::filesystem::path nextBackupPath(const std::filesystem::path& projectFilePath) {
  std::filesystem::path firstCandidate(projectFilePath);
  firstCandidate += ".v3.bak";
  if (!QFile::exists(core::toQString(firstCandidate))) {
    return firstCandidate;
  }

  for (int suffix = 1;; ++suffix) {
    std::filesystem::path candidate(projectFilePath);
    candidate += ".v3." + std::to_string(suffix) + ".bak";
    if (!QFile::exists(core::toQString(candidate))) {
      return candidate;
    }
  }
}

}  // namespace

ProjectMigrationReport ProjectMigration::analyze(const QDomDocument& doc,
                                                 const std::filesystem::path& projectFilePath) {
  ProjectMigrationReport report;
  const QDomElement projectEl(doc.documentElement());
  bool versionOk = false;
  report.sourceVersion = projectEl.attribute("version").toInt(&versionOk);
  if (!versionOk || report.sourceVersion != 3) {
    return report;
  }

  QSet<QString> imagesWithTrim;
  const QDomElement filtersEl(projectEl.namedItem("filters").toElement());
  const QDomElement fixOrientationEl(filtersEl.namedItem("fix-orientation").toElement());
  for (QDomNode node = fixOrientationEl.firstChild(); !node.isNull(); node = node.nextSibling()) {
    if (!node.isElement() || node.nodeName() != "image") {
      continue;
    }
    const QDomElement imageEl(node.toElement());
    if (!imageEl.namedItem("trim").isNull()) {
      imagesWithTrim.insert(imageEl.attribute("id"));
    }
  }

  const QDomElement imagesEl(projectEl.namedItem("images").toElement());
  for (QDomNode node = imagesEl.firstChild(); !node.isNull(); node = node.nextSibling()) {
    if (!node.isElement() || node.nodeName() != "image") {
      continue;
    }
    if (!imagesWithTrim.contains(node.toElement().attribute("id"))) {
      ++report.imagesWithDefaultTrim;
    }
  }

  const QDomNodeList colorSettings(doc.elementsByTagName("color-or-grayscale"));
  for (int i = 0; i < colorSettings.size(); ++i) {
    if (!colorSettings.at(i).toElement().hasAttribute("fillOutsidePageBox")) {
      ++report.colorSettingsWithDefaultFillOutsidePageBox;
    }
  }

  const QDomNodeList deskewFilters(doc.elementsByTagName("deskew"));
  for (int i = 0; i < deskewFilters.size(); ++i) {
    const QDomElement deskewEl(deskewFilters.at(i).toElement());
    for (QDomNode pageNode = deskewEl.firstChild(); !pageNode.isNull(); pageNode = pageNode.nextSibling()) {
      if (!pageNode.isElement() || pageNode.nodeName() != "page") {
        continue;
      }

      const QDomElement paramsEl(pageNode.namedItem("params").toElement());
      if (paramsEl.isNull() || paramsEl.hasAttribute("oblique-mode") || paramsEl.attribute("oblique-disabled") == "1") {
        continue;
      }

      if (paramsEl.hasAttribute("autoOblique")) {
        ++report.obliqueModesFromAutoOblique;
        if (paramsEl.attribute("autoOblique") != "0") {
          ++report.obliqueAutoModesFromAutoOblique;
        } else {
          ++report.obliqueManualModesFromAutoOblique;
        }
      } else if (!paramsEl.hasAttribute("oblique")) {
        ++report.obliqueModesDisabledForMissingAttributes;
      } else {
        ++report.obliqueModesFromDeskewMode;
        if (paramsEl.attribute("mode") != "manual") {
          ++report.obliqueAutoModesFromDeskewMode;
        } else {
          ++report.obliqueManualModesFromDeskewMode;
        }
      }
    }
  }

  const QDir projectDir(QFileInfo(core::toQString(projectFilePath)).absolutePath());
  if (willBeWrittenRelative(projectEl.attribute("outputDirectory"), projectDir)) {
    ++report.pathsWrittenRelative;
  }
  const QDomElement directoriesEl(projectEl.namedItem("directories").toElement());
  for (QDomNode node = directoriesEl.firstChild(); !node.isNull(); node = node.nextSibling()) {
    if (node.isElement() && node.nodeName() == "directory"
        && willBeWrittenRelative(node.toElement().attribute("path"), projectDir)) {
      ++report.pathsWrittenRelative;
    }
  }

  return report;
}

bool ProjectMigration::createBackupForFormat3(const std::filesystem::path& projectFilePath,
                                              std::filesystem::path* backupFilePath,
                                              QString* errorMessage) {
  if (backupFilePath) {
    *backupFilePath = std::filesystem::path{};
  }
  if (errorMessage) {
    errorMessage->clear();
  }

  const QString qtProjectFilePath(core::toQString(projectFilePath));
  QFile projectFile(qtProjectFilePath);
  if (!projectFile.exists()) {
    return true;
  }
  if (!projectFile.open(QIODevice::ReadOnly)) {
    if (errorMessage) {
      *errorMessage = projectFile.errorString();
    }
    return false;
  }

  QDomDocument doc;
  if (!doc.setContent(&projectFile)) {
    return true;
  }
  bool versionOk = false;
  const int version = doc.documentElement().attribute("version").toInt(&versionOk);
  projectFile.close();
  if (!versionOk || version != 3 || doc.documentElement().tagName() != "project") {
    return true;
  }

  const std::filesystem::path backupPath(nextBackupPath(projectFilePath));
  const QString qtBackupPath(core::toQString(backupPath));
  if (!projectFile.copy(qtBackupPath)) {
    if (errorMessage) {
      *errorMessage = QString("Could not create backup %1: %2").arg(qtBackupPath, projectFile.errorString());
    }
    return false;
  }

  if (backupFilePath) {
    *backupFilePath = backupPath;
  }
  return true;
}
