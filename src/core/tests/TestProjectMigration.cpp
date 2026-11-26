// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QtXml>
#include <catch2/catch_test_macros.hpp>

#include "ImageFileInfo.h"
#include "ImageInfo.h"
#include "OutputFileNameGenerator.h"
#include "PathUtils.h"
#include "ProjectBackup.h"
#include "ProjectMigration.h"
#include "ProjectPages.h"
#include "ProjectReader.h"
#include "ProjectWriter.h"
#include "version.h"

namespace {

QDomDocument makeReaderProject(int version) {
  QDomDocument doc;
  QDomElement projectEl(doc.createElement("project"));
  projectEl.setAttribute("version", version);
  projectEl.setAttribute("outputDirectory", "output");
  doc.appendChild(projectEl);

  QDomElement directoriesEl(doc.createElement("directories"));
  QDomElement directoryEl(doc.createElement("directory"));
  directoryEl.setAttribute("id", 1);
  directoryEl.setAttribute("path", ".");
  directoriesEl.appendChild(directoryEl);
  projectEl.appendChild(directoriesEl);

  QDomElement filesEl(doc.createElement("files"));
  QDomElement fileEl(doc.createElement("file"));
  fileEl.setAttribute("id", 1);
  fileEl.setAttribute("dirId", 1);
  fileEl.setAttribute("name", "page.tif");
  filesEl.appendChild(fileEl);
  projectEl.appendChild(filesEl);

  QDomElement imagesEl(doc.createElement("images"));
  QDomElement imageEl(doc.createElement("image"));
  imageEl.setAttribute("id", 1);
  imageEl.setAttribute("subPages", 1);
  imageEl.setAttribute("fileId", 1);
  imageEl.setAttribute("fileImage", 0);
  imagesEl.appendChild(imageEl);
  projectEl.appendChild(imagesEl);

  QDomElement pagesEl(doc.createElement("pages"));
  QDomElement pageEl(doc.createElement("page"));
  pageEl.setAttribute("id", 1);
  pageEl.setAttribute("imageId", 1);
  pageEl.setAttribute("subPage", "single");
  pagesEl.appendChild(pageEl);
  projectEl.appendChild(pagesEl);
  return doc;
}

QDomDocument makeMigrationProject(const QString& projectDir) {
  QDomDocument doc;
  QDomElement projectEl(doc.createElement("project"));
  projectEl.setAttribute("version", 3);
  projectEl.setAttribute("outputDirectory", QDir(projectDir).filePath("output"));
  doc.appendChild(projectEl);

  QDomElement directoriesEl(doc.createElement("directories"));
  QDomElement directoryEl(doc.createElement("directory"));
  directoryEl.setAttribute("id", 1);
  directoryEl.setAttribute("path", QDir(projectDir).filePath("source"));
  directoriesEl.appendChild(directoryEl);
  projectEl.appendChild(directoriesEl);

  QDomElement imagesEl(doc.createElement("images"));
  for (int id = 1; id <= 2; ++id) {
    QDomElement imageEl(doc.createElement("image"));
    imageEl.setAttribute("id", id);
    imagesEl.appendChild(imageEl);
  }
  projectEl.appendChild(imagesEl);

  QDomElement filtersEl(doc.createElement("filters"));
  QDomElement fixOrientationEl(doc.createElement("fix-orientation"));
  QDomElement imageWithTrimEl(doc.createElement("image"));
  imageWithTrimEl.setAttribute("id", 1);
  imageWithTrimEl.appendChild(doc.createElement("trim"));
  fixOrientationEl.appendChild(imageWithTrimEl);
  QDomElement imageWithoutTrimEl(doc.createElement("image"));
  imageWithoutTrimEl.setAttribute("id", 2);
  fixOrientationEl.appendChild(imageWithoutTrimEl);
  filtersEl.appendChild(fixOrientationEl);

  QDomElement colorWithDefaultEl(doc.createElement("color-or-grayscale"));
  filtersEl.appendChild(colorWithDefaultEl);
  QDomElement colorWithValueEl(doc.createElement("color-or-grayscale"));
  colorWithValueEl.setAttribute("fillOutsidePageBox", "0");
  filtersEl.appendChild(colorWithValueEl);

  QDomElement deskewEl(doc.createElement("deskew"));
  int deskewPageId = 1;
  const auto appendDeskewPage = [&](const QString& attribute, const QString& value, const bool hasOblique = false) {
    QDomElement pageEl(doc.createElement("page"));
    pageEl.setAttribute("id", deskewPageId++);
    QDomElement paramsEl(doc.createElement("params"));
    if (!attribute.isNull()) {
      paramsEl.setAttribute(attribute, value);
    }
    if (hasOblique) {
      paramsEl.setAttribute("oblique", "0");
    }
    pageEl.appendChild(paramsEl);
    deskewEl.appendChild(pageEl);
  };
  appendDeskewPage("autoOblique", "0");
  appendDeskewPage("autoOblique", "1");
  appendDeskewPage("mode", "manual", true);
  appendDeskewPage(QString(), QString());
  appendDeskewPage("mode", "manual");
  appendDeskewPage("mode", "auto", true);
  appendDeskewPage("oblique-mode", "manual");
  filtersEl.appendChild(deskewEl);

  projectEl.appendChild(filtersEl);

  return doc;
}

bool writeFile(const QString& path, const QByteArray& contents) {
  QFile file(path);
  return file.open(QIODevice::WriteOnly) && file.write(contents) == contents.size();
}

QByteArray readFile(const QString& path) {
  QFile file(path);
  if (!file.open(QIODevice::ReadOnly)) {
    return QByteArray();
  }
  return file.readAll();
}

}  // namespace


TEST_CASE("CoreProjectMigrationTestSuite / test_reader_accepts_format_3_and_current_format") {
  QTemporaryDir tempDir;
  REQUIRE(tempDir.isValid());
  const QString projectPath(tempDir.filePath("project.ScanTailor"));

  const ProjectReader legacyReader(makeReaderProject(3), core::toFileSystemPath(projectPath));
  CHECK(legacyReader.success());
  CHECK((legacyReader.migrationReport().sourceVersion) == (3));

  const ProjectReader currentReader(makeReaderProject(PROJECT_VERSION), core::toFileSystemPath(projectPath));
  CHECK(currentReader.success());
  CHECK((currentReader.migrationReport().sourceVersion) == (PROJECT_VERSION));
}

TEST_CASE("CoreProjectMigrationTestSuite / test_reader_does_not_modify_legacy_xml") {
  QTemporaryDir tempDir;
  REQUIRE(tempDir.isValid());

  const QDomDocument doc(makeReaderProject(3));
  const QString originalXml(doc.toString());
  const ProjectReader reader(doc, core::toFileSystemPath(tempDir.filePath("project.ScanTailor")));

  REQUIRE(reader.success());
  CHECK(doc.toString() == originalXml);
}

TEST_CASE("CoreProjectMigrationTestSuite / test_reader_rejects_unsupported_versions") {
  QTemporaryDir tempDir;
  REQUIRE(tempDir.isValid());
  const QString projectPath(tempDir.filePath("project.ScanTailor"));

  const ProjectReader olderReader(makeReaderProject(2), core::toFileSystemPath(projectPath));
  const ProjectReader newerReader(makeReaderProject(PROJECT_VERSION + 1), core::toFileSystemPath(projectPath));
  CHECK(!olderReader.success());
  CHECK(!newerReader.success());
}

TEST_CASE("CoreProjectMigrationTestSuite / test_reader_rejects_missing_or_nonnumeric_version") {
  QTemporaryDir tempDir;
  REQUIRE(tempDir.isValid());
  const QString projectPath(tempDir.filePath("project.ScanTailor"));

  QDomDocument missingVersionDoc(makeReaderProject(3));
  missingVersionDoc.documentElement().removeAttribute("version");
  const ProjectReader missingVersionReader(missingVersionDoc, core::toFileSystemPath(projectPath));
  CHECK(!missingVersionReader.success());

  QDomDocument nonnumericVersionDoc(makeReaderProject(3));
  nonnumericVersionDoc.documentElement().setAttribute("version", "three");
  const ProjectReader nonnumericVersionReader(nonnumericVersionDoc, core::toFileSystemPath(projectPath));
  CHECK(!nonnumericVersionReader.success());
}

TEST_CASE("CoreProjectMigrationTestSuite / test_migration_report_covers_legacy_defaults_and_relative_paths") {
  QTemporaryDir tempDir;
  REQUIRE(tempDir.isValid());

  const ProjectMigrationReport report(ProjectMigration::analyze(
      makeMigrationProject(tempDir.path()), core::toFileSystemPath(tempDir.filePath("project.ScanTailor"))));

  CHECK((report.sourceVersion) == (3));
  CHECK((report.imagesWithDefaultTrim) == (1));
  CHECK((report.colorSettingsWithDefaultFillOutsidePageBox) == (1));
  CHECK((report.obliqueModesFromAutoOblique) == (2));
  CHECK((report.obliqueAutoModesFromAutoOblique) == (1));
  CHECK((report.obliqueManualModesFromAutoOblique) == (1));
  CHECK((report.obliqueModesFromDeskewMode) == (2));
  CHECK((report.obliqueAutoModesFromDeskewMode) == (1));
  CHECK((report.obliqueManualModesFromDeskewMode) == (1));
  CHECK((report.obliqueModesDisabledForMissingAttributes) == (2));
  CHECK((report.pathsWrittenRelative) == (2));
}

TEST_CASE("CoreProjectMigrationTestSuite / test_backup_preserves_original_and_avoids_existing_backups") {
  QTemporaryDir tempDir;
  REQUIRE(tempDir.isValid());

  const QString projectPath(tempDir.filePath("legacy.ScanTailor"));
  const QByteArray projectContents("<project version=\"3\"/>");
  REQUIRE(writeFile(projectPath, projectContents));

  std::filesystem::path firstBackup;
  QString errorMessage;
  REQUIRE(ProjectMigration::createBackupForFormat3(core::toFileSystemPath(projectPath), &firstBackup, &errorMessage));
  CHECK(readFile(core::toQString(firstBackup)) == projectContents);

  const QByteArray sentinel("keep this backup");
  REQUIRE(writeFile(core::toQString(firstBackup), sentinel));
  std::filesystem::path secondBackup;
  REQUIRE(ProjectMigration::createBackupForFormat3(core::toFileSystemPath(projectPath), &secondBackup, &errorMessage));

  CHECK(firstBackup != secondBackup);
  CHECK(readFile(core::toQString(firstBackup)) == sentinel);
  CHECK(readFile(core::toQString(secondBackup)) == projectContents);
}

TEST_CASE("CoreProjectMigrationTestSuite / test_backup_skips_non_v3_and_nonexistent_targets") {
  QTemporaryDir tempDir;
  REQUIRE(tempDir.isValid());

  const QString currentPath(tempDir.filePath("current.ScanTailor"));
  REQUIRE(writeFile(currentPath, QByteArray("<project version=\"4\"/>")));
  std::filesystem::path backupPath("unexpected");
  QString errorMessage;
  CHECK(ProjectMigration::createBackupForFormat3(core::toFileSystemPath(currentPath), &backupPath, &errorMessage));
  CHECK(backupPath.empty());

  const QString nonexistentPath(tempDir.filePath("new.ScanTailor"));
  CHECK(ProjectMigration::createBackupForFormat3(core::toFileSystemPath(nonexistentPath), &backupPath, &errorMessage));
  CHECK(backupPath.empty());
}

TEST_CASE("CoreProjectMigrationTestSuite / test_project_backup_retention_keeps_latest_versions") {
  QTemporaryDir tempDir;
  REQUIRE(tempDir.isValid());

  const QString projectPath(tempDir.filePath("project.ScanTailor"));
  QString errorMessage;
  for (int version = 0; version < 3; ++version) {
    REQUIRE(writeFile(projectPath, QByteArray("version-") + QByteArray::number(version)));
    REQUIRE(createProjectBackup(core::toFileSystemPath(projectPath), 2, &errorMessage));
  }

  const QDir backupsDirectory(projectPath + QStringLiteral(".backups"));
  const QStringList backups = backupsDirectory.entryList({QStringLiteral("*.ScanTailor")}, QDir::Files, QDir::Name);
  REQUIRE(backups.size() == 2);
  CHECK(readFile(backupsDirectory.filePath(backups.at(0))) == QByteArray("version-1"));
  CHECK(readFile(backupsDirectory.filePath(backups.at(1))) == QByteArray("version-2"));

  REQUIRE(writeFile(projectPath, QByteArray("version-3")));
  REQUIRE(createProjectBackup(core::toFileSystemPath(projectPath), 0, &errorMessage));
  CHECK(backupsDirectory.entryList({QStringLiteral("*.ScanTailor")}, QDir::Files, QDir::Name).size() == 2);
}

TEST_CASE("CoreProjectMigrationTestSuite / test_v3_save_writes_v4_with_relative_paths_and_preserves_backup") {
  QTemporaryDir tempDir;
  REQUIRE(tempDir.isValid());
  const QDir rootDir(tempDir.path());
  const QString projectDirPath(rootDir.filePath("project"));
  REQUIRE(QDir().mkpath(projectDirPath));
  REQUIRE(QDir().mkpath(rootDir.filePath("source")));
  REQUIRE(QDir().mkpath(rootDir.filePath("output")));

  const QDir projectDir(projectDirPath);
  const QString sourcePath(rootDir.filePath("source/page.tif"));
  REQUIRE(writeFile(sourcePath, QByteArray()));

  const QString projectPath(projectDir.filePath("project.ScanTailor"));
  const QByteArray originalV3("<project version=\"3\"/>");
  REQUIRE(writeFile(projectPath, originalV3));

  const ImageInfo imageInfo(ImageId(core::toFileSystemPath(sourcePath), 0),
                            ImageMetadata(QSize(100, 100), Dpi(300, 300)), 1, false, false);
  const auto pages = std::make_shared<ProjectPages>(std::vector<ImageInfo>{imageInfo}, Qt::LeftToRight);
  const OutputFileNameGenerator outputFileNameGenerator(
      std::make_shared<FileNameDisambiguator>(), core::toFileSystemPath(rootDir.filePath("output")), Qt::LeftToRight);
  const ProjectWriter writer(pages, SelectedPage(), outputFileNameGenerator);
  const std::vector<ProjectWriter::FilterPtr> filters;

  QString errorMessage;
  const bool writeSucceeded = writer.write(core::toFileSystemPath(projectPath), filters, &errorMessage, 2);
  {
    INFO(errorMessage.toStdString());
    REQUIRE(writeSucceeded);
  }
  CHECK(readFile(projectPath + ".v3.bak") == originalV3);
  const QDir historyDirectory(projectPath + QStringLiteral(".backups"));
  QStringList historyBackups = historyDirectory.entryList({QStringLiteral("*.ScanTailor")}, QDir::Files, QDir::Name);
  REQUIRE(historyBackups.size() == 1);
  CHECK(readFile(historyDirectory.filePath(historyBackups.front())) == originalV3);

  REQUIRE(writer.write(core::toFileSystemPath(projectPath), filters, &errorMessage, 2));
  historyBackups = historyDirectory.entryList({QStringLiteral("*.ScanTailor")}, QDir::Files, QDir::Name);
  CHECK(historyBackups.size() == 1);

  QFile savedProject(projectPath);
  REQUIRE(savedProject.open(QIODevice::ReadOnly));
  QDomDocument savedDoc;
  REQUIRE(savedDoc.setContent(&savedProject));
  const QDomElement rootEl(savedDoc.documentElement());
  CHECK((rootEl.attribute("version").toInt()) == (PROJECT_VERSION));
  CHECK(rootEl.attribute("outputDirectory") == "../output");
  const QDomElement directoryEl(rootEl.namedItem("directories").firstChildElement("directory"));
  CHECK(directoryEl.attribute("path") == "../source");
}

TEST_CASE("CoreProjectMigrationTestSuite / test_project_dpi_override_survives_save_and_reload") {
  QTemporaryDir tempDir;
  REQUIRE(tempDir.isValid());
  const QDir rootDir(tempDir.path());
  const QString projectDirPath(rootDir.filePath("project"));
  REQUIRE(QDir().mkpath(projectDirPath));
  const QString sourcePath(QDir(projectDirPath).filePath("page.tif"));
  const QString projectPath(QDir(projectDirPath).filePath("project.ScanTailor"));
  REQUIRE(writeFile(sourcePath, QByteArray()));

  const ImageId imageId(core::toFileSystemPath(sourcePath), 0);
  const ImageMetadata originalMetadata(QSize(100, 120), Dpi(300, 300));
  const auto pages = std::make_shared<ProjectPages>(
      std::vector<ImageInfo>{ImageInfo(imageId, originalMetadata, 1, false, false)}, Qt::LeftToRight);
  const OutputFileNameGenerator outputFileNameGenerator(
      std::make_shared<FileNameDisambiguator>(), core::toFileSystemPath(rootDir.filePath("output")), Qt::LeftToRight);
  const ProjectWriter initialWriter(pages, SelectedPage(), outputFileNameGenerator);
  QString errorMessage;
  REQUIRE(initialWriter.write(core::toFileSystemPath(projectPath), {}, &errorMessage));

  QDomDocument initialDoc;
  {
    QFile initialProject(projectPath);
    REQUIRE(initialProject.open(QIODevice::ReadOnly));
    REQUIRE(initialDoc.setContent(&initialProject));
  }
  CHECK(initialDoc.documentElement().namedItem("directories").firstChildElement("directory").attribute("path") == ".");
  const ProjectReader loadedProject(initialDoc, core::toFileSystemPath(projectPath));
  REQUIRE(loadedProject.success());
  const std::shared_ptr<ProjectPages>& loadedPages = loadedProject.pages();
  std::vector<ImageFileInfo> files = loadedPages->toImageFileInfo();
  REQUIRE(files.size() == 1);
  files.front().imageInfo().front().setDpi(Dpi(400, 400));
  loadedPages->updateMetadataFrom(files);
  REQUIRE(loadedPages->toPageSequence(PAGE_VIEW).pageAt(0).metadata().dpi() == Dpi(400, 400));

  const ProjectWriter updatedWriter(loadedPages, loadedProject.selectedPage(), outputFileNameGenerator);
  REQUIRE(updatedWriter.write(core::toFileSystemPath(projectPath), {}, &errorMessage));

  QFile savedProject(projectPath);
  REQUIRE(savedProject.open(QIODevice::ReadOnly));
  QDomDocument savedDoc;
  REQUIRE(savedDoc.setContent(&savedProject));
  const QDomElement dpiElement
      = savedDoc.documentElement().namedItem("images").firstChildElement("image").firstChildElement("dpi");
  REQUIRE_FALSE(dpiElement.isNull());
  CHECK(dpiElement.attribute("horizontal") == "400");
  CHECK(dpiElement.attribute("vertical") == "400");

  const ProjectReader reader(savedDoc, core::toFileSystemPath(projectPath));
  REQUIRE(reader.success());
  CHECK(reader.pages()->toPageSequence(PAGE_VIEW).pageAt(0).metadata().dpi() == Dpi(400, 400));
}

#if defined(Q_OS_UNIX)
TEST_CASE("CoreProjectMigrationTestSuite / test_v3_save_keeps_absolute_paths_when_relative_paths_cross_symlinks") {
  QTemporaryDir tempDir;
  REQUIRE(tempDir.isValid());
  const QDir rootDir(tempDir.path());
  const QString realProjectDir(rootDir.filePath("real/project"));
  REQUIRE(QDir().mkpath(realProjectDir));
  const QString sourceDir(rootDir.filePath("real/source"));
  REQUIRE(QDir().mkpath(sourceDir));
  const QString outputDir(rootDir.filePath("real/output"));
  REQUIRE(QDir().mkpath(outputDir));

  const QString projectAlias(rootDir.filePath("project-link"));
  REQUIRE(QFile::link(realProjectDir, projectAlias));

  const QString sourcePath(QDir(sourceDir).filePath("page.tif"));
  REQUIRE(writeFile(sourcePath, QByteArray()));
  const QString projectPath(QDir(projectAlias).filePath("project.ScanTailor"));
  const QByteArray originalV3("<project version=\"3\"/>");
  REQUIRE(writeFile(projectPath, originalV3));

  const ImageInfo imageInfo(ImageId(core::toFileSystemPath(sourcePath), 0),
                            ImageMetadata(QSize(100, 100), Dpi(300, 300)), 1, false, false);
  const auto pages = std::make_shared<ProjectPages>(std::vector<ImageInfo>{imageInfo}, Qt::LeftToRight);
  const OutputFileNameGenerator outputFileNameGenerator(std::make_shared<FileNameDisambiguator>(),
                                                        core::toFileSystemPath(outputDir), Qt::LeftToRight);
  const ProjectWriter writer(pages, SelectedPage(), outputFileNameGenerator);
  const std::vector<ProjectWriter::FilterPtr> filters;

  QString errorMessage;
  const bool writeSucceeded = writer.write(core::toFileSystemPath(projectPath), filters, &errorMessage);
  {
    INFO(errorMessage.toStdString());
    REQUIRE(writeSucceeded);
  }

  QFile savedProject(projectPath);
  REQUIRE(savedProject.open(QIODevice::ReadOnly));
  QDomDocument savedDoc;
  REQUIRE(savedDoc.setContent(&savedProject));
  const QDomElement rootEl(savedDoc.documentElement());
  CHECK(QDir::isAbsolutePath(rootEl.attribute("outputDirectory")));
  const QDomElement directoryEl(rootEl.namedItem("directories").firstChildElement("directory"));
  CHECK(QDir::isAbsolutePath(directoryEl.attribute("path")));
}
#endif
