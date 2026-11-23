// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QtXml>
#include <catch2/catch_test_macros.hpp>

#include "ImageInfo.h"
#include "OutputFileNameGenerator.h"
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
  const auto appendDeskewPage = [&](const QString& attribute, const QString& value) {
    QDomElement pageEl(doc.createElement("page"));
    pageEl.setAttribute("id", deskewPageId++);
    QDomElement paramsEl(doc.createElement("params"));
    if (!attribute.isNull()) {
      paramsEl.setAttribute(attribute, value);
    }
    pageEl.appendChild(paramsEl);
    deskewEl.appendChild(pageEl);
  };
  appendDeskewPage("autoOblique", "0");
  appendDeskewPage("autoOblique", "1");
  appendDeskewPage("mode", "manual");
  appendDeskewPage(QString(), QString());
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

  const ProjectReader legacyReader(makeReaderProject(3), projectPath);
  CHECK(legacyReader.success());
  CHECK((legacyReader.migrationReport().sourceVersion) == (3));

  const ProjectReader currentReader(makeReaderProject(PROJECT_VERSION), projectPath);
  CHECK(currentReader.success());
  CHECK((currentReader.migrationReport().sourceVersion) == (PROJECT_VERSION));
}

TEST_CASE("CoreProjectMigrationTestSuite / test_reader_does_not_modify_legacy_xml") {
  QTemporaryDir tempDir;
  REQUIRE(tempDir.isValid());

  const QDomDocument doc(makeReaderProject(3));
  const QString originalXml(doc.toString());
  const ProjectReader reader(doc, tempDir.filePath("project.ScanTailor"));

  REQUIRE(reader.success());
  CHECK(doc.toString() == originalXml);
}

TEST_CASE("CoreProjectMigrationTestSuite / test_reader_rejects_unsupported_versions") {
  QTemporaryDir tempDir;
  REQUIRE(tempDir.isValid());
  const QString projectPath(tempDir.filePath("project.ScanTailor"));

  const ProjectReader olderReader(makeReaderProject(2), projectPath);
  const ProjectReader newerReader(makeReaderProject(PROJECT_VERSION + 1), projectPath);
  CHECK(!olderReader.success());
  CHECK(!newerReader.success());
}

TEST_CASE("CoreProjectMigrationTestSuite / test_reader_rejects_missing_or_nonnumeric_version") {
  QTemporaryDir tempDir;
  REQUIRE(tempDir.isValid());
  const QString projectPath(tempDir.filePath("project.ScanTailor"));

  QDomDocument missingVersionDoc(makeReaderProject(3));
  missingVersionDoc.documentElement().removeAttribute("version");
  const ProjectReader missingVersionReader(missingVersionDoc, projectPath);
  CHECK(!missingVersionReader.success());

  QDomDocument nonnumericVersionDoc(makeReaderProject(3));
  nonnumericVersionDoc.documentElement().setAttribute("version", "three");
  const ProjectReader nonnumericVersionReader(nonnumericVersionDoc, projectPath);
  CHECK(!nonnumericVersionReader.success());
}

TEST_CASE("CoreProjectMigrationTestSuite / test_migration_report_covers_legacy_defaults_and_relative_paths") {
  QTemporaryDir tempDir;
  REQUIRE(tempDir.isValid());

  const ProjectMigrationReport report(
      ProjectMigration::analyze(makeMigrationProject(tempDir.path()), tempDir.filePath("project.ScanTailor")));

  CHECK((report.sourceVersion) == (3));
  CHECK((report.imagesWithDefaultTrim) == (1));
  CHECK((report.colorSettingsWithDefaultFillOutsidePageBox) == (1));
  CHECK((report.obliqueModesFromAutoOblique) == (2));
  CHECK((report.obliqueAutoModesFromAutoOblique) == (1));
  CHECK((report.obliqueManualModesFromAutoOblique) == (1));
  CHECK((report.obliqueModesFromDeskewMode) == (2));
  CHECK((report.obliqueAutoModesFromDeskewMode) == (1));
  CHECK((report.obliqueManualModesFromDeskewMode) == (1));
  CHECK((report.pathsWrittenRelative) == (2));
}

TEST_CASE("CoreProjectMigrationTestSuite / test_backup_preserves_original_and_avoids_existing_backups") {
  QTemporaryDir tempDir;
  REQUIRE(tempDir.isValid());

  const QString projectPath(tempDir.filePath("legacy.ScanTailor"));
  const QByteArray projectContents("<project version=\"3\"/>");
  REQUIRE(writeFile(projectPath, projectContents));

  QString firstBackup;
  QString errorMessage;
  REQUIRE(ProjectMigration::createBackupForFormat3(projectPath, &firstBackup, &errorMessage));
  CHECK(readFile(firstBackup) == projectContents);

  const QByteArray sentinel("keep this backup");
  REQUIRE(writeFile(firstBackup, sentinel));
  QString secondBackup;
  REQUIRE(ProjectMigration::createBackupForFormat3(projectPath, &secondBackup, &errorMessage));

  CHECK(firstBackup != secondBackup);
  CHECK(readFile(firstBackup) == sentinel);
  CHECK(readFile(secondBackup) == projectContents);
}

TEST_CASE("CoreProjectMigrationTestSuite / test_backup_skips_non_v3_and_nonexistent_targets") {
  QTemporaryDir tempDir;
  REQUIRE(tempDir.isValid());

  const QString currentPath(tempDir.filePath("current.ScanTailor"));
  REQUIRE(writeFile(currentPath, QByteArray("<project version=\"4\"/>")));
  QString backupPath("unexpected");
  QString errorMessage;
  CHECK(ProjectMigration::createBackupForFormat3(currentPath, &backupPath, &errorMessage));
  CHECK(backupPath.isEmpty());

  const QString nonexistentPath(tempDir.filePath("new.ScanTailor"));
  CHECK(ProjectMigration::createBackupForFormat3(nonexistentPath, &backupPath, &errorMessage));
  CHECK(backupPath.isEmpty());
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

  const ImageInfo imageInfo(ImageId(sourcePath, 0), ImageMetadata(QSize(100, 100), Dpi(300, 300)), 1, false, false);
  const auto pages = std::make_shared<ProjectPages>(std::vector<ImageInfo>{imageInfo}, Qt::LeftToRight);
  const OutputFileNameGenerator outputFileNameGenerator(std::make_shared<FileNameDisambiguator>(),
                                                        rootDir.filePath("output"), Qt::LeftToRight);
  const ProjectWriter writer(pages, SelectedPage(), outputFileNameGenerator);
  const std::vector<ProjectWriter::FilterPtr> filters;

  QString errorMessage;
  {
    INFO(errorMessage.toStdString());
    REQUIRE(writer.write(projectPath, filters, &errorMessage));
  }
  CHECK(readFile(projectPath + ".v3.bak") == originalV3);

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

  const ImageInfo imageInfo(ImageId(sourcePath, 0), ImageMetadata(QSize(100, 100), Dpi(300, 300)), 1, false, false);
  const auto pages = std::make_shared<ProjectPages>(std::vector<ImageInfo>{imageInfo}, Qt::LeftToRight);
  const OutputFileNameGenerator outputFileNameGenerator(std::make_shared<FileNameDisambiguator>(), outputDir,
                                                        Qt::LeftToRight);
  const ProjectWriter writer(pages, SelectedPage(), outputFileNameGenerator);
  const std::vector<ProjectWriter::FilterPtr> filters;

  QString errorMessage;
  {
    INFO(errorMessage.toStdString());
    REQUIRE(writer.write(projectPath, filters, &errorMessage));
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
