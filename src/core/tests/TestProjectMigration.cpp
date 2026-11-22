// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QtXml>
#include <boost/test/unit_test.hpp>

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

  QDomElement deskewWithAutoObliqueEl(doc.createElement("deskew"));
  QDomElement autoObliqueParamsEl(doc.createElement("params"));
  autoObliqueParamsEl.setAttribute("autoOblique", "0");
  deskewWithAutoObliqueEl.appendChild(autoObliqueParamsEl);
  filtersEl.appendChild(deskewWithAutoObliqueEl);

  QDomElement deskewWithAutomaticObliqueEl(doc.createElement("deskew"));
  QDomElement automaticObliqueParamsEl(doc.createElement("params"));
  automaticObliqueParamsEl.setAttribute("autoOblique", "1");
  deskewWithAutomaticObliqueEl.appendChild(automaticObliqueParamsEl);
  filtersEl.appendChild(deskewWithAutomaticObliqueEl);

  QDomElement deskewWithModeEl(doc.createElement("deskew"));
  QDomElement modeParamsEl(doc.createElement("params"));
  modeParamsEl.setAttribute("mode", "manual");
  deskewWithModeEl.appendChild(modeParamsEl);
  filtersEl.appendChild(deskewWithModeEl);

  QDomElement deskewWithDefaultModeEl(doc.createElement("deskew"));
  deskewWithDefaultModeEl.appendChild(doc.createElement("params"));
  filtersEl.appendChild(deskewWithDefaultModeEl);

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

BOOST_AUTO_TEST_SUITE(CoreProjectMigrationTestSuite)

BOOST_AUTO_TEST_CASE(test_reader_accepts_format_3_and_current_format) {
  QTemporaryDir tempDir;
  BOOST_REQUIRE(tempDir.isValid());
  const QString projectPath(tempDir.filePath("project.ScanTailor"));

  const ProjectReader legacyReader(makeReaderProject(3), projectPath);
  BOOST_CHECK(legacyReader.success());
  BOOST_CHECK_EQUAL(legacyReader.migrationReport().sourceVersion, 3);

  const ProjectReader currentReader(makeReaderProject(PROJECT_VERSION), projectPath);
  BOOST_CHECK(currentReader.success());
  BOOST_CHECK_EQUAL(currentReader.migrationReport().sourceVersion, PROJECT_VERSION);
}

BOOST_AUTO_TEST_CASE(test_reader_does_not_modify_legacy_xml) {
  QTemporaryDir tempDir;
  BOOST_REQUIRE(tempDir.isValid());

  const QDomDocument doc(makeReaderProject(3));
  const QString originalXml(doc.toString());
  const ProjectReader reader(doc, tempDir.filePath("project.ScanTailor"));

  BOOST_REQUIRE(reader.success());
  BOOST_CHECK(doc.toString() == originalXml);
}

BOOST_AUTO_TEST_CASE(test_reader_rejects_unsupported_versions) {
  QTemporaryDir tempDir;
  BOOST_REQUIRE(tempDir.isValid());
  const QString projectPath(tempDir.filePath("project.ScanTailor"));

  const ProjectReader olderReader(makeReaderProject(2), projectPath);
  const ProjectReader newerReader(makeReaderProject(PROJECT_VERSION + 1), projectPath);
  BOOST_CHECK(!olderReader.success());
  BOOST_CHECK(!newerReader.success());
}

BOOST_AUTO_TEST_CASE(test_reader_rejects_missing_or_nonnumeric_version) {
  QTemporaryDir tempDir;
  BOOST_REQUIRE(tempDir.isValid());
  const QString projectPath(tempDir.filePath("project.ScanTailor"));

  QDomDocument missingVersionDoc(makeReaderProject(3));
  missingVersionDoc.documentElement().removeAttribute("version");
  const ProjectReader missingVersionReader(missingVersionDoc, projectPath);
  BOOST_CHECK(!missingVersionReader.success());

  QDomDocument nonnumericVersionDoc(makeReaderProject(3));
  nonnumericVersionDoc.documentElement().setAttribute("version", "three");
  const ProjectReader nonnumericVersionReader(nonnumericVersionDoc, projectPath);
  BOOST_CHECK(!nonnumericVersionReader.success());
}

BOOST_AUTO_TEST_CASE(test_migration_report_covers_legacy_defaults_and_relative_paths) {
  QTemporaryDir tempDir;
  BOOST_REQUIRE(tempDir.isValid());

  const ProjectMigrationReport report(
      ProjectMigration::analyze(makeMigrationProject(tempDir.path()), tempDir.filePath("project.ScanTailor")));

  BOOST_CHECK_EQUAL(report.sourceVersion, 3);
  BOOST_CHECK_EQUAL(report.imagesWithDefaultTrim, 1);
  BOOST_CHECK_EQUAL(report.colorSettingsWithDefaultFillOutsidePageBox, 1);
  BOOST_CHECK_EQUAL(report.obliqueModesFromAutoOblique, 2);
  BOOST_CHECK_EQUAL(report.obliqueAutoModesFromAutoOblique, 1);
  BOOST_CHECK_EQUAL(report.obliqueManualModesFromAutoOblique, 1);
  BOOST_CHECK_EQUAL(report.obliqueModesFromDeskewMode, 2);
  BOOST_CHECK_EQUAL(report.obliqueAutoModesFromDeskewMode, 1);
  BOOST_CHECK_EQUAL(report.obliqueManualModesFromDeskewMode, 1);
  BOOST_CHECK_EQUAL(report.pathsWrittenRelative, 2);
}

BOOST_AUTO_TEST_CASE(test_backup_preserves_original_and_avoids_existing_backups) {
  QTemporaryDir tempDir;
  BOOST_REQUIRE(tempDir.isValid());

  const QString projectPath(tempDir.filePath("legacy.ScanTailor"));
  const QByteArray projectContents("<project version=\"3\"/>");
  BOOST_REQUIRE(writeFile(projectPath, projectContents));

  QString firstBackup;
  QString errorMessage;
  BOOST_REQUIRE(ProjectMigration::createBackupForFormat3(projectPath, &firstBackup, &errorMessage));
  BOOST_CHECK(readFile(firstBackup) == projectContents);

  const QByteArray sentinel("keep this backup");
  BOOST_REQUIRE(writeFile(firstBackup, sentinel));
  QString secondBackup;
  BOOST_REQUIRE(ProjectMigration::createBackupForFormat3(projectPath, &secondBackup, &errorMessage));

  BOOST_CHECK(firstBackup != secondBackup);
  BOOST_CHECK(readFile(firstBackup) == sentinel);
  BOOST_CHECK(readFile(secondBackup) == projectContents);
}

BOOST_AUTO_TEST_CASE(test_backup_skips_non_v3_and_nonexistent_targets) {
  QTemporaryDir tempDir;
  BOOST_REQUIRE(tempDir.isValid());

  const QString currentPath(tempDir.filePath("current.ScanTailor"));
  BOOST_REQUIRE(writeFile(currentPath, QByteArray("<project version=\"4\"/>")));
  QString backupPath("unexpected");
  QString errorMessage;
  BOOST_CHECK(ProjectMigration::createBackupForFormat3(currentPath, &backupPath, &errorMessage));
  BOOST_CHECK(backupPath.isEmpty());

  const QString nonexistentPath(tempDir.filePath("new.ScanTailor"));
  BOOST_CHECK(ProjectMigration::createBackupForFormat3(nonexistentPath, &backupPath, &errorMessage));
  BOOST_CHECK(backupPath.isEmpty());
}

BOOST_AUTO_TEST_CASE(test_v3_save_writes_v4_with_relative_paths_and_preserves_backup) {
  QTemporaryDir tempDir;
  BOOST_REQUIRE(tempDir.isValid());
  const QDir rootDir(tempDir.path());
  const QString projectDirPath(rootDir.filePath("project"));
  BOOST_REQUIRE(QDir().mkpath(projectDirPath));
  BOOST_REQUIRE(QDir().mkpath(rootDir.filePath("source")));
  BOOST_REQUIRE(QDir().mkpath(rootDir.filePath("output")));

  const QDir projectDir(projectDirPath);
  const QString sourcePath(rootDir.filePath("source/page.tif"));
  BOOST_REQUIRE(writeFile(sourcePath, QByteArray()));

  const QString projectPath(projectDir.filePath("project.ScanTailor"));
  const QByteArray originalV3("<project version=\"3\"/>");
  BOOST_REQUIRE(writeFile(projectPath, originalV3));

  const ImageInfo imageInfo(ImageId(sourcePath, 0), ImageMetadata(QSize(100, 100), Dpi(300, 300)), 1, false, false);
  const auto pages = std::make_shared<ProjectPages>(std::vector<ImageInfo>{imageInfo}, Qt::LeftToRight);
  const OutputFileNameGenerator outputFileNameGenerator(std::make_shared<FileNameDisambiguator>(),
                                                        rootDir.filePath("output"), Qt::LeftToRight);
  const ProjectWriter writer(pages, SelectedPage(), outputFileNameGenerator);
  const std::vector<ProjectWriter::FilterPtr> filters;

  QString errorMessage;
  BOOST_REQUIRE_MESSAGE(writer.write(projectPath, filters, &errorMessage), errorMessage.toStdString());
  BOOST_CHECK(readFile(projectPath + ".v3.bak") == originalV3);

  QFile savedProject(projectPath);
  BOOST_REQUIRE(savedProject.open(QIODevice::ReadOnly));
  QDomDocument savedDoc;
  BOOST_REQUIRE(savedDoc.setContent(&savedProject));
  const QDomElement rootEl(savedDoc.documentElement());
  BOOST_CHECK_EQUAL(rootEl.attribute("version").toInt(), PROJECT_VERSION);
  BOOST_CHECK(rootEl.attribute("outputDirectory") == "../output");
  const QDomElement directoryEl(rootEl.namedItem("directories").firstChildElement("directory"));
  BOOST_CHECK(directoryEl.attribute("path") == "../source");
}

BOOST_AUTO_TEST_SUITE_END()
