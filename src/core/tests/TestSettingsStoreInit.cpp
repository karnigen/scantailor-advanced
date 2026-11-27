// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#include <config.h>
#include <core/DefaultParams.h>
#include <core/DefaultParamsProfileManager.h>
#include <core/PathUtils.h>
#include <core/SettingsStore.h>

#include <toml++/toml.h>
#include <QCoreApplication>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QProcessEnvironment>
#include <QSettings>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QThread>
#include <catch2/catch_test_macros.hpp>

#include <limits>
#include <string_view>

TEST_CASE("SettingsStoreTestSuite / test_explicit_relative_config_directory_and_profiles",
          "[settings-store-explicit]") {
  QTemporaryDir tempDir;
  REQUIRE(tempDir.isValid());

  QFile blocker(tempDir.filePath("not-a-directory"));
  REQUIRE(blocker.open(QIODevice::WriteOnly));
  blocker.close();

  QString error;
  CHECK_FALSE(SettingsStore::initialize(QDir(blocker.fileName()).filePath("child"), true, &error));
  CHECK_FALSE(error.isEmpty());

  const QString currentDirectory = QDir::currentPath();
  REQUIRE(QDir::setCurrent(tempDir.path()));
  REQUIRE(SettingsStore::initialize(QStringLiteral("relative/config"), true, &error));
  REQUIRE(error.isEmpty());
  QDir::setCurrent(currentDirectory);

  const QString expectedDirectory = QDir(tempDir.path()).filePath(QStringLiteral("relative/config"));
  CHECK(SettingsStore::configDirectory() == core::toFileSystemPath(expectedDirectory));
  CHECK(SettingsStore::configFilePath()
        == core::toFileSystemPath(QDir(expectedDirectory).filePath(QStringLiteral("config.toml"))));
  CHECK(SettingsStore::profilesDirectory()
        == core::toFileSystemPath(QDir(expectedDirectory).filePath(QStringLiteral("profiles"))));
  CHECK(SettingsStore::configDirectoryExplicit());
  CHECK(SettingsStore::configDirectoryWritable());

  SettingsStore settings;
  settings.setValue(QStringLiteral("settings/enabled"), true);
  CHECK(QFileInfo::exists(core::toQString(SettingsStore::configFilePath())));
  CHECK_FALSE(QFileInfo::exists(QDir(expectedDirectory).filePath(QStringLiteral("config.ini"))));

  DefaultParamsProfileManager profileManager;
  const auto profile = profileManager.createDefaultProfile();
  REQUIRE(profileManager.writeProfile(*profile, QStringLiteral("test-profile")));
  CHECK(QFileInfo::exists(
      QDir(core::toQString(SettingsStore::profilesDirectory())).filePath(QStringLiteral("test-profile.stp"))));
}

TEST_CASE("SettingsStoreTestSuite / test_copy_and_migrate_existing_standard_ini", "[settings-store-legacy]") {
  QTemporaryDir tempDir;
  REQUIRE(tempDir.isValid());

  const QString configRoot = QDir(tempDir.path()).filePath(QStringLiteral("platform-config"));
  qputenv("HOME", tempDir.path().toLocal8Bit());
  qputenv("APPDATA", configRoot.toLocal8Bit());
  qputenv("LOCALAPPDATA", configRoot.toLocal8Bit());
  qputenv("XDG_CONFIG_HOME", configRoot.toLocal8Bit());
  QSettings::setPath(QSettings::IniFormat, QSettings::UserScope,
                     QDir(tempDir.path()).filePath(QStringLiteral("legacy-settings")));

  const QString applicationName = QString::fromUtf8(APPLICATION_NAME);
  QCoreApplication::setOrganizationName(applicationName);
  QCoreApplication::setApplicationName(applicationName);
  QSettings legacySettings(QSettings::IniFormat, QSettings::UserScope, applicationName, applicationName);
  legacySettings.setValue(QStringLiteral("settings/enable_opengl"), true);
  legacySettings.setValue(QStringLiteral("window/geometry"), QByteArray::fromHex("0a0b0c"));
  legacySettings.sync();
  REQUIRE(legacySettings.status() == QSettings::NoError);
  const QString legacyFilePath = legacySettings.fileName();

  const QString defaultDirectory = core::toQString(SettingsStore::defaultConfigDirectory());
  const QString relativeDefaultDirectory = QDir(tempDir.path()).relativeFilePath(defaultDirectory);
  CHECK(!relativeDefaultDirectory.startsWith(QStringLiteral("..")));

  QString error;
  REQUIRE(SettingsStore::initialize(QString(), false, &error, true));
  REQUIRE(error.isEmpty());
  CHECK(SettingsStore::configFilePath()
        == core::toFileSystemPath(QDir(defaultDirectory).filePath(QStringLiteral("config.toml"))));
  CHECK(QFileInfo::exists(legacyFilePath));
  CHECK_FALSE(QFileInfo::exists(QDir(defaultDirectory).filePath(QStringLiteral("config.ini"))));
  CHECK(QFileInfo::exists(QDir(defaultDirectory).filePath(QStringLiteral("config.ini.bak"))));
  CHECK(QFileInfo::exists(core::toQString(SettingsStore::configFilePath())));

  SettingsStore settings;
  CHECK(settings.boolValue(QStringLiteral("settings/enable_opengl")));
  CHECK(settings.byteArrayValue(QStringLiteral("window/geometry")) == QByteArray::fromHex("0a0b0c"));
}

TEST_CASE("SettingsStoreTestSuite / test_migrate_ini_to_toml_and_rename_source_to_backup",
          "[settings-store-migration]") {
  QTemporaryDir tempDir;
  REQUIRE(tempDir.isValid());
  const QString iniPath = tempDir.filePath(QStringLiteral("config.ini"));
  const QByteArray geometry = QByteArray::fromHex("01020aff");

  {
    QSettings legacy(iniPath, QSettings::IniFormat);
    legacy.setValue(QStringLiteral("settings/enable_opengl"), true);
    legacy.setValue(QStringLiteral("settings/thumbnail_quality"), QSize(120, 80));
    legacy.setValue(QStringLiteral("window/nonMaximizedGeometry"), geometry);
    legacy.setValue(QStringLiteral("custom/unknown"), QStringLiteral("preserve me"));
    legacy.setValue(QStringLiteral("CollapsibleGroupBox/alignmentGroup/collapsed"), false);
    legacy.setValue(QStringLiteral("CollapsibleGroupBox/colorOperationsOptions/collapsed"), true);
    legacy.beginWriteArray(QStringLiteral("project/recent"));
    legacy.setArrayIndex(0);
    legacy.setValue(QStringLiteral("path"), QStringLiteral("/scans/first.ScanTailor"));
    legacy.setArrayIndex(1);
    legacy.setValue(QStringLiteral("path"), QStringLiteral("/scans/second.ScanTailor"));
    legacy.endArray();
    legacy.sync();
    REQUIRE(legacy.status() == QSettings::NoError);
  }
  QFile legacyFile(iniPath);
  REQUIRE(legacyFile.open(QIODevice::ReadOnly));
  const QByteArray originalIni = legacyFile.readAll();
  legacyFile.close();

  QString error;
  REQUIRE(SettingsStore::initialize(tempDir.path(), true, &error));
  REQUIRE(error.isEmpty());
  CHECK(SettingsStore::configFilePath() == core::toFileSystemPath(tempDir.filePath(QStringLiteral("config.toml"))));
  const QString backupPath = iniPath + QStringLiteral(".bak");
  CHECK_FALSE(QFileInfo::exists(iniPath));
  CHECK(QFileInfo::exists(backupPath));

  SettingsStore settings;
  CHECK(settings.boolValue(QStringLiteral("settings/enable_opengl")));
  CHECK(settings.sizeValue(QStringLiteral("settings/thumbnail_quality")) == QSize(120, 80));
  CHECK(settings.byteArrayValue(QStringLiteral("window/nonMaximizedGeometry")) == geometry);
  CHECK(settings.stringValue(QStringLiteral("custom/unknown")) == QStringLiteral("preserve me"));
  CHECK_FALSE(settings.boolValue(QStringLiteral("CollapsibleGroupBox/alignmentGroup/collapsed")));
  CHECK(settings.boolValue(QStringLiteral("CollapsibleGroupBox/colorOperationsOptions/collapsed")));
  CHECK((settings.stringList(QStringLiteral("project/recent"))
         == QStringList{QStringLiteral("/scans/first.ScanTailor"), QStringLiteral("/scans/second.ScanTailor")}));

  QFile backup(backupPath);
  REQUIRE(backup.open(QIODevice::ReadOnly));
  CHECK(backup.readAll() == originalIni);

  QFile tomlFile(core::toQString(SettingsStore::configFilePath()));
  REQUIRE(tomlFile.open(QIODevice::ReadOnly));
  const QByteArray tomlContents = tomlFile.readAll();
  CHECK(tomlContents.contains("alignmentGroup = { collapsed = false }"));
  CHECK(tomlContents.contains("colorOperationsOptions = { collapsed = true }"));

  settings.setValue(QStringLiteral("settings/enable_opengl"), false);
  CHECK_FALSE(settings.boolValue(QStringLiteral("settings/enable_opengl")));
  QFile updatedToml(core::toQString(SettingsStore::configFilePath()));
  REQUIRE(updatedToml.open(QIODevice::ReadOnly));
  CHECK(updatedToml.readAll().contains("enable_opengl = false"));
}

TEST_CASE("SettingsStoreTestSuite / test_empty_ini_migration_writes_schema_document",
          "[settings-store-empty-ini-migration]") {
  QTemporaryDir tempDir;
  REQUIRE(tempDir.isValid());
  const QString iniPath = tempDir.filePath(QStringLiteral("config.ini"));
  QFile emptyIni(iniPath);
  REQUIRE(emptyIni.open(QIODevice::WriteOnly));
  emptyIni.close();

  QString error;
  REQUIRE(SettingsStore::initialize(tempDir.path(), true, &error));
  REQUIRE(error.isEmpty());
  CHECK(QFileInfo::exists(tempDir.filePath(QStringLiteral("config.toml"))));
  CHECK_FALSE(QFileInfo::exists(iniPath));
  CHECK(QFileInfo::exists(iniPath + QStringLiteral(".bak")));
  QFile toml(tempDir.filePath(QStringLiteral("config.toml")));
  REQUIRE(toml.open(QIODevice::ReadOnly));
  CHECK(toml.readAll().contains("schema_version = 1"));
}

TEST_CASE("SettingsStoreTestSuite / test_malformed_ini_is_not_archived", "[settings-store-invalid-ini]") {
  QTemporaryDir tempDir;
  REQUIRE(tempDir.isValid());
  const QString iniPath = tempDir.filePath(QStringLiteral("config.ini"));
  const QByteArray invalidIni("[invalid\nsetting = true\n");
  QFile ini(iniPath);
  REQUIRE(ini.open(QIODevice::WriteOnly));
  REQUIRE(ini.write(invalidIni) == invalidIni.size());
  ini.close();

  QString error;
  CHECK_FALSE(SettingsStore::initialize(tempDir.path(), true, &error));
  CHECK(error.contains(QStringLiteral("legacy INI")));
  CHECK(QFileInfo::exists(iniPath));
  CHECK_FALSE(QFileInfo::exists(tempDir.filePath(QStringLiteral("config.toml"))));
  QFile unchangedIni(iniPath);
  REQUIRE(unchangedIni.open(QIODevice::ReadOnly));
  CHECK(unchangedIni.readAll() == invalidIni);
}

TEST_CASE("SettingsStoreTestSuite / test_invalid_migration_candidate_keeps_ini",
          "[settings-store-invalid-migration-candidate]") {
  QTemporaryDir tempDir;
  REQUIRE(tempDir.isValid());
  const QString iniPath = tempDir.filePath(QStringLiteral("config.ini"));
  {
    QSettings legacy(iniPath, QSettings::IniFormat);
    legacy.setValue(QStringLiteral("schema_version"), QStringLiteral("not a version"));
    legacy.sync();
    REQUIRE(legacy.status() == QSettings::NoError);
  }
  QFile source(iniPath);
  REQUIRE(source.open(QIODevice::ReadOnly));
  const QByteArray originalIni = source.readAll();
  source.close();

  QString error;
  CHECK_FALSE(SettingsStore::initialize(tempDir.path(), true, &error));
  CHECK(error.contains(QStringLiteral("schema_version")));
  CHECK(QFileInfo::exists(iniPath));
  CHECK_FALSE(QFileInfo::exists(tempDir.filePath(QStringLiteral("config.toml"))));
  QFile unchangedIni(iniPath);
  REQUIRE(unchangedIni.open(QIODevice::ReadOnly));
  CHECK(unchangedIni.readAll() == originalIni);
}

TEST_CASE("SettingsStoreTestSuite / test_migration_rejects_fractional_integer", "[settings-store-fractional-integer]") {
  QTemporaryDir tempDir;
  REQUIRE(tempDir.isValid());
  const QString iniPath = tempDir.filePath(QStringLiteral("config.ini"));
  QSettings legacy(iniPath, QSettings::IniFormat);
  legacy.setValue(QStringLiteral("settings/bw_compression"), 1.5);
  legacy.sync();
  REQUIRE(legacy.status() == QSettings::NoError);

  QString error;
  CHECK_FALSE(SettingsStore::initialize(tempDir.path(), true, &error));
  CHECK(error.contains(QStringLiteral("settings/bw_compression")));
  CHECK(QFileInfo::exists(iniPath));
  CHECK_FALSE(QFileInfo::exists(tempDir.filePath(QStringLiteral("config.toml"))));
}

TEST_CASE("SettingsStoreTestSuite / test_migration_rejects_nonnumeric_value", "[settings-store-nonnumeric-value]") {
  QTemporaryDir tempDir;
  REQUIRE(tempDir.isValid());
  const QString iniPath = tempDir.filePath(QStringLiteral("config.ini"));
  QSettings legacy(iniPath, QSettings::IniFormat);
  legacy.setValue(QStringLiteral("settings/deskew_deviation_coef"), QStringLiteral("not a number"));
  legacy.sync();
  REQUIRE(legacy.status() == QSettings::NoError);

  QString error;
  CHECK_FALSE(SettingsStore::initialize(tempDir.path(), true, &error));
  CHECK(error.contains(QStringLiteral("settings/deskew_deviation_coef")));
  CHECK(QFileInfo::exists(iniPath));
  CHECK_FALSE(QFileInfo::exists(tempDir.filePath(QStringLiteral("config.toml"))));
}

TEST_CASE("SettingsStoreTestSuite / test_migration_rejects_nonfinite_number", "[settings-store-nonfinite-number]") {
  QTemporaryDir tempDir;
  REQUIRE(tempDir.isValid());
  const QString iniPath = tempDir.filePath(QStringLiteral("config.ini"));
  QSettings legacy(iniPath, QSettings::IniFormat);
  legacy.setValue(QStringLiteral("settings/deskew_deviation_coef"), std::numeric_limits<double>::infinity());
  legacy.sync();
  REQUIRE(legacy.status() == QSettings::NoError);

  QString error;
  CHECK_FALSE(SettingsStore::initialize(tempDir.path(), true, &error));
  CHECK(error.contains(QStringLiteral("settings/deskew_deviation_coef")));
  CHECK(QFileInfo::exists(iniPath));
  CHECK_FALSE(QFileInfo::exists(tempDir.filePath(QStringLiteral("config.toml"))));
}

TEST_CASE("SettingsStoreTestSuite / test_migration_rejects_invalid_size_type", "[settings-store-invalid-size-type]") {
  QTemporaryDir tempDir;
  REQUIRE(tempDir.isValid());
  const QString iniPath = tempDir.filePath(QStringLiteral("config.ini"));
  QSettings legacy(iniPath, QSettings::IniFormat);
  legacy.setValue(QStringLiteral("settings/thumbnail_quality"), QStringLiteral("not a size"));
  legacy.sync();
  REQUIRE(legacy.status() == QSettings::NoError);

  QString error;
  CHECK_FALSE(SettingsStore::initialize(tempDir.path(), true, &error));
  CHECK(error.contains(QStringLiteral("settings/thumbnail_quality")));
  CHECK(QFileInfo::exists(iniPath));
  CHECK_FALSE(QFileInfo::exists(tempDir.filePath(QStringLiteral("config.toml"))));
}

TEST_CASE("SettingsStoreTestSuite / test_migration_rejects_nonpositive_sizef", "[settings-store-nonpositive-sizef]") {
  QTemporaryDir tempDir;
  REQUIRE(tempDir.isValid());
  const QString iniPath = tempDir.filePath(QStringLiteral("config.ini"));
  QSettings legacy(iniPath, QSettings::IniFormat);
  legacy.setValue(QStringLiteral("settings/max_logical_thumb_size"), QSizeF(-1.0, 20.0));
  legacy.sync();
  REQUIRE(legacy.status() == QSettings::NoError);

  QString error;
  CHECK_FALSE(SettingsStore::initialize(tempDir.path(), true, &error));
  CHECK(error.contains(QStringLiteral("settings/max_logical_thumb_size")));
  CHECK(QFileInfo::exists(iniPath));
  CHECK_FALSE(QFileInfo::exists(tempDir.filePath(QStringLiteral("config.toml"))));
}

TEST_CASE("SettingsStoreTestSuite / test_migration_rejects_nonfinite_sizef",
          "[settings-store-nonfinite-sizef-migration]") {
  QTemporaryDir tempDir;
  REQUIRE(tempDir.isValid());
  const QString iniPath = tempDir.filePath(QStringLiteral("config.ini"));
  QSettings legacy(iniPath, QSettings::IniFormat);
  legacy.setValue(QStringLiteral("settings/max_logical_thumb_size"),
                  QSizeF(std::numeric_limits<double>::infinity(), 20.0));
  legacy.sync();
  REQUIRE(legacy.status() == QSettings::NoError);

  QString error;
  CHECK_FALSE(SettingsStore::initialize(tempDir.path(), true, &error));
  CHECK(error.contains(QStringLiteral("settings/max_logical_thumb_size")));
  CHECK(QFileInfo::exists(iniPath));
  CHECK_FALSE(QFileInfo::exists(tempDir.filePath(QStringLiteral("config.toml"))));
}

TEST_CASE("SettingsStoreTestSuite / test_toml_rejects_nonfinite_numeric_setting", "[settings-store-nonfinite-double]") {
  QTemporaryDir tempDir;
  REQUIRE(tempDir.isValid());
  QFile toml(tempDir.filePath(QStringLiteral("config.toml")));
  REQUIRE(toml.open(QIODevice::WriteOnly));
  REQUIRE(toml.write("schema_version = 1\n\n[settings]\ndeskew_deviation_coef = nan\n") > 0);
  toml.close();

  QString error;
  CHECK_FALSE(SettingsStore::initialize(tempDir.path(), true, &error));
  CHECK(error.contains(QStringLiteral("settings/deskew_deviation_coef")));
}

TEST_CASE("SettingsStoreTestSuite / test_toml_rejects_nonfinite_sizef_component", "[settings-store-nonfinite-sizef]") {
  QTemporaryDir tempDir;
  REQUIRE(tempDir.isValid());
  QFile toml(tempDir.filePath(QStringLiteral("config.toml")));
  REQUIRE(toml.open(QIODevice::WriteOnly));
  REQUIRE(toml.write("schema_version = 1\n\n[settings]\nmax_logical_thumb_size = [inf, 20.0]\n") > 0);
  toml.close();

  QString error;
  CHECK_FALSE(SettingsStore::initialize(tempDir.path(), true, &error));
  CHECK(error.contains(QStringLiteral("settings/max_logical_thumb_size")));
}

TEST_CASE("SettingsStoreTestSuite / test_toml_rejects_nonpositive_size", "[settings-store-nonpositive-size]") {
  QTemporaryDir tempDir;
  REQUIRE(tempDir.isValid());
  QFile toml(tempDir.filePath(QStringLiteral("config.toml")));
  REQUIRE(toml.open(QIODevice::WriteOnly));
  REQUIRE(toml.write("schema_version = 1\n\n[settings]\nthumbnail_quality = [0, 80]\n") > 0);
  toml.close();

  QString error;
  CHECK_FALSE(SettingsStore::initialize(tempDir.path(), true, &error));
  CHECK(error.contains(QStringLiteral("settings/thumbnail_quality")));
}

TEST_CASE("SettingsStoreTestSuite / test_toml_rejects_nonpositive_sizef", "[settings-store-nonpositive-sizef-toml]") {
  QTemporaryDir tempDir;
  REQUIRE(tempDir.isValid());
  QFile toml(tempDir.filePath(QStringLiteral("config.toml")));
  REQUIRE(toml.open(QIODevice::WriteOnly));
  REQUIRE(toml.write("schema_version = 1\n\n[settings]\nmax_logical_thumb_size = [0.0, 20.0]\n") > 0);
  toml.close();

  QString error;
  CHECK_FALSE(SettingsStore::initialize(tempDir.path(), true, &error));
  CHECK(error.contains(QStringLiteral("settings/max_logical_thumb_size")));
}

TEST_CASE("SettingsStoreTestSuite / test_migration_does_not_overwrite_existing_ini_backup",
          "[settings-store-migration-backup]") {
  QTemporaryDir tempDir;
  REQUIRE(tempDir.isValid());
  const QString iniPath = tempDir.filePath(QStringLiteral("config.ini"));
  const QString backupPath = iniPath + QStringLiteral(".bak");
  const QByteArray existingBackup("keep this backup");

  QSettings legacy(iniPath, QSettings::IniFormat);
  legacy.setValue(QStringLiteral("settings/enable_opengl"), true);
  legacy.sync();
  REQUIRE(legacy.status() == QSettings::NoError);
  QFile backup(backupPath);
  REQUIRE(backup.open(QIODevice::WriteOnly));
  REQUIRE(backup.write(existingBackup) == existingBackup.size());
  backup.close();

  QString error;
  REQUIRE(SettingsStore::initialize(tempDir.path(), true, &error));
  CHECK_FALSE(error.isEmpty());
  CHECK(QFileInfo::exists(iniPath));
  QFile preservedBackup(backupPath);
  REQUIRE(preservedBackup.open(QIODevice::ReadOnly));
  CHECK(preservedBackup.readAll() == existingBackup);
  CHECK(QFileInfo::exists(tempDir.filePath(QStringLiteral("config.toml"))));
}

TEST_CASE("SettingsStoreTestSuite / test_concurrent_migration_processes", "[settings-store-migration-race]") {
  if (qEnvironmentVariable("SETTINGS_STORE_MIGRATION_CHILD") == QStringLiteral("1")) {
    const QString directory = qEnvironmentVariable("SETTINGS_STORE_MIGRATION_DIRECTORY");
    const QString readyPath = qEnvironmentVariable("SETTINGS_STORE_MIGRATION_READY");
    const QString releasePath = qEnvironmentVariable("SETTINGS_STORE_MIGRATION_RELEASE");
    QFile ready(readyPath);
    REQUIRE(ready.open(QIODevice::WriteOnly));
    ready.close();

    QElapsedTimer waitTimer;
    waitTimer.start();
    while (!QFileInfo::exists(releasePath) && waitTimer.elapsed() < 10000) {
      QThread::msleep(10);
    }
    REQUIRE(QFileInfo::exists(releasePath));

    QString error;
    REQUIRE(SettingsStore::initialize(directory, true, &error));
    CHECK(error.isEmpty());
    return;
  }

  QTemporaryDir tempDir;
  REQUIRE(tempDir.isValid());
  QSettings legacy(tempDir.filePath(QStringLiteral("config.ini")), QSettings::IniFormat);
  legacy.setValue(QStringLiteral("settings/enable_opengl"), true);
  legacy.sync();
  REQUIRE(legacy.status() == QSettings::NoError);

  const QString releasePath = tempDir.filePath(QStringLiteral("release"));
  QProcess processes[2];
  for (int i = 0; i < 2; ++i) {
    const QString readyPath = tempDir.filePath(QStringLiteral("ready-%1").arg(i));
    QProcessEnvironment environment = QProcessEnvironment::systemEnvironment();
    environment.insert(QStringLiteral("SETTINGS_STORE_MIGRATION_CHILD"), QStringLiteral("1"));
    environment.insert(QStringLiteral("SETTINGS_STORE_MIGRATION_DIRECTORY"), tempDir.path());
    environment.insert(QStringLiteral("SETTINGS_STORE_MIGRATION_READY"), readyPath);
    environment.insert(QStringLiteral("SETTINGS_STORE_MIGRATION_RELEASE"), releasePath);
    processes[i].setProcessEnvironment(environment);
    processes[i].setProgram(qEnvironmentVariable("SETTINGS_STORE_TEST_EXECUTABLE"));
    processes[i].setArguments({QStringLiteral("[settings-store-migration-race]")});
    processes[i].start();
    REQUIRE(processes[i].waitForStarted(5000));
  }

  QElapsedTimer waitTimer;
  waitTimer.start();
  while ((!QFileInfo::exists(tempDir.filePath(QStringLiteral("ready-0")))
          || !QFileInfo::exists(tempDir.filePath(QStringLiteral("ready-1"))))
         && waitTimer.elapsed() < 10000) {
    QThread::msleep(10);
  }
  CHECK(QFileInfo::exists(tempDir.filePath(QStringLiteral("ready-0"))));
  CHECK(QFileInfo::exists(tempDir.filePath(QStringLiteral("ready-1"))));
  QFile release(releasePath);
  REQUIRE(release.open(QIODevice::WriteOnly));
  release.close();

  for (QProcess& process : processes) {
    REQUIRE(process.waitForFinished(15000));
    CHECK(process.exitStatus() == QProcess::NormalExit);
    CHECK(process.exitCode() == 0);
  }
  CHECK(QFileInfo::exists(tempDir.filePath(QStringLiteral("config.toml"))));
  CHECK(QFileInfo::exists(tempDir.filePath(QStringLiteral("config.ini.bak"))));
  CHECK_FALSE(QFileInfo::exists(tempDir.filePath(QStringLiteral("config.ini"))));
}

TEST_CASE("SettingsStoreTestSuite / test_concurrent_process_writes_preserve_both_updates",
          "[settings-store-concurrent-write]") {
  if (qEnvironmentVariable("SETTINGS_STORE_WRITE_CHILD") == QStringLiteral("1")) {
    SettingsStore settings(core::toFileSystemPath(qEnvironmentVariable("SETTINGS_STORE_WRITE_PATH")));
    const QString readyPath = qEnvironmentVariable("SETTINGS_STORE_WRITE_READY");
    const QString releasePath = qEnvironmentVariable("SETTINGS_STORE_WRITE_RELEASE");
    QFile ready(readyPath);
    REQUIRE(ready.open(QIODevice::WriteOnly));
    ready.close();

    QElapsedTimer waitTimer;
    waitTimer.start();
    while (!QFileInfo::exists(releasePath) && waitTimer.elapsed() < 10000) {
      QThread::msleep(10);
    }
    REQUIRE(QFileInfo::exists(releasePath));

    settings.setValue(qEnvironmentVariable("SETTINGS_STORE_WRITE_KEY"), true);
    CHECK_FALSE(settings.hasError());
    return;
  }

  QTemporaryDir tempDir;
  REQUIRE(tempDir.isValid());
  const QString tomlPath = tempDir.filePath(QStringLiteral("config.toml"));
  {
    SettingsStore settings(core::toFileSystemPath(tomlPath));
    settings.setValue(QStringLiteral("settings/initialized"), true);
  }
  SettingsStore observer(core::toFileSystemPath(tomlPath));

  const QString releasePath = tempDir.filePath(QStringLiteral("write-release"));
  QProcess processes[2];
  const QStringList keys{QStringLiteral("settings/firstProcess"), QStringLiteral("settings/secondProcess")};
  for (int i = 0; i < 2; ++i) {
    const QString readyPath = tempDir.filePath(QStringLiteral("write-ready-%1").arg(i));
    QProcessEnvironment environment = QProcessEnvironment::systemEnvironment();
    environment.insert(QStringLiteral("SETTINGS_STORE_WRITE_CHILD"), QStringLiteral("1"));
    environment.insert(QStringLiteral("SETTINGS_STORE_WRITE_PATH"), tomlPath);
    environment.insert(QStringLiteral("SETTINGS_STORE_WRITE_READY"), readyPath);
    environment.insert(QStringLiteral("SETTINGS_STORE_WRITE_RELEASE"), releasePath);
    environment.insert(QStringLiteral("SETTINGS_STORE_WRITE_KEY"), keys.at(i));
    processes[i].setProcessEnvironment(environment);
    processes[i].setProgram(qEnvironmentVariable("SETTINGS_STORE_TEST_EXECUTABLE"));
    processes[i].setArguments({QStringLiteral("[settings-store-concurrent-write]")});
    processes[i].start();
    REQUIRE(processes[i].waitForStarted(5000));
  }

  QElapsedTimer waitTimer;
  waitTimer.start();
  while ((!QFileInfo::exists(tempDir.filePath(QStringLiteral("write-ready-0")))
          || !QFileInfo::exists(tempDir.filePath(QStringLiteral("write-ready-1"))))
         && waitTimer.elapsed() < 10000) {
    QThread::msleep(10);
  }
  CHECK(QFileInfo::exists(tempDir.filePath(QStringLiteral("write-ready-0"))));
  CHECK(QFileInfo::exists(tempDir.filePath(QStringLiteral("write-ready-1"))));
  QFile release(releasePath);
  REQUIRE(release.open(QIODevice::WriteOnly));
  release.close();

  for (QProcess& process : processes) {
    REQUIRE(process.waitForFinished(15000));
    CHECK(process.exitStatus() == QProcess::NormalExit);
    CHECK(process.exitCode() == 0);
  }

  QFile persistedFile(tomlPath);
  REQUIRE(persistedFile.open(QIODevice::ReadOnly));
  const QByteArray persistedContents = persistedFile.readAll();
  persistedFile.close();
  observer.sync();
  CHECK_FALSE(observer.hasError());
  CHECK(observer.boolValue(keys.at(0)));
  CHECK(observer.boolValue(keys.at(1)));
  QFile unchangedPersistedFile(tomlPath);
  REQUIRE(unchangedPersistedFile.open(QIODevice::ReadOnly));
  CHECK(unchangedPersistedFile.readAll() == persistedContents);
}

TEST_CASE("SettingsStoreTestSuite / test_toml_takes_precedence_over_ini", "[settings-store-precedence]") {
  QTemporaryDir tempDir;
  REQUIRE(tempDir.isValid());

  QSettings legacy(tempDir.filePath(QStringLiteral("config.ini")), QSettings::IniFormat);
  legacy.setValue(QStringLiteral("settings/enable_opengl"), true);
  legacy.sync();
  REQUIRE(legacy.status() == QSettings::NoError);

  QFile toml(tempDir.filePath(QStringLiteral("config.toml")));
  REQUIRE(toml.open(QIODevice::WriteOnly | QIODevice::Text));
  REQUIRE(toml.write("schema_version = 1\n\n[settings]\nenable_opengl = false\n\n[custom]\nretained = \"keep\"\n") > 0);
  toml.close();

  QString error;
  REQUIRE(SettingsStore::initialize(tempDir.path(), true, &error));
  REQUIRE(error.isEmpty());
  {
    SettingsStore settings;
    CHECK_FALSE(settings.boolValue(QStringLiteral("settings/enable_opengl")));
    settings.setValue(QStringLiteral("project/lastDir"), QStringLiteral("/scans"));
  }
  QFile savedToml(tempDir.filePath(QStringLiteral("config.toml")));
  REQUIRE(savedToml.open(QIODevice::ReadOnly));
  const QByteArray savedContents = savedToml.readAll();
  const toml::table savedDocument
      = toml::parse(std::string_view(savedContents.constData(), static_cast<std::size_t>(savedContents.size())));
  const auto lastDir = savedDocument["project"]["lastDir"].value<std::string>();
  const auto retained = savedDocument["custom"]["retained"].value<std::string>();
  REQUIRE(lastDir);
  REQUIRE(retained);
  CHECK(*lastDir == "/scans");
  CHECK(*retained == "keep");
  CHECK(QFileInfo::exists(tempDir.filePath(QStringLiteral("config.ini"))));
}

TEST_CASE("SettingsStoreTestSuite / test_invalid_toml_does_not_fall_back_to_ini", "[settings-store-invalid-toml]") {
  QTemporaryDir tempDir;
  REQUIRE(tempDir.isValid());

  QSettings legacy(tempDir.filePath(QStringLiteral("config.ini")), QSettings::IniFormat);
  legacy.setValue(QStringLiteral("settings/enable_opengl"), true);
  legacy.sync();
  REQUIRE(legacy.status() == QSettings::NoError);

  const QByteArray invalidContents("schema_version = [\n");
  QFile toml(tempDir.filePath(QStringLiteral("config.toml")));
  REQUIRE(toml.open(QIODevice::WriteOnly));
  REQUIRE(toml.write(invalidContents) == invalidContents.size());
  toml.close();

  QString error;
  CHECK_FALSE(SettingsStore::initialize(tempDir.path(), true, &error));
  CHECK_FALSE(error.isEmpty());
  QFile unchangedToml(toml.fileName());
  REQUIRE(unchangedToml.open(QIODevice::ReadOnly));
  CHECK(unchangedToml.readAll() == invalidContents);
  CHECK(QFileInfo::exists(tempDir.filePath(QStringLiteral("config.ini"))));
}

TEST_CASE("SettingsStoreTestSuite / test_wrong_toml_value_type_is_reported", "[settings-store-invalid-type]") {
  QTemporaryDir tempDir;
  REQUIRE(tempDir.isValid());

  QSettings legacy(tempDir.filePath(QStringLiteral("config.ini")), QSettings::IniFormat);
  legacy.setValue(QStringLiteral("settings/enable_opengl"), true);
  legacy.sync();
  REQUIRE(legacy.status() == QSettings::NoError);

  QFile toml(tempDir.filePath(QStringLiteral("config.toml")));
  REQUIRE(toml.open(QIODevice::WriteOnly | QIODevice::Text));
  REQUIRE(toml.write("schema_version = 1\n\n[settings]\nenable_opengl = \"not a boolean\"\n") > 0);
  toml.close();

  QString error;
  CHECK_FALSE(SettingsStore::initialize(tempDir.path(), true, &error));
  CHECK(error.contains(QStringLiteral("settings/enable_opengl")));
  CHECK(QFileInfo::exists(tempDir.filePath(QStringLiteral("config.ini"))));
}

TEST_CASE("SettingsStoreTestSuite / test_schema_version_requires_native_integer", "[settings-store-schema-type]") {
  QTemporaryDir tempDir;
  REQUIRE(tempDir.isValid());
  QFile toml(tempDir.filePath(QStringLiteral("config.toml")));
  REQUIRE(toml.open(QIODevice::WriteOnly));
  REQUIRE(toml.write("schema_version = 1.0\n") > 0);
  toml.close();

  QString error;
  CHECK_FALSE(SettingsStore::initialize(tempDir.path(), true, &error));
  CHECK(error.contains(QStringLiteral("schema_version")));
}

TEST_CASE("SettingsStoreTestSuite / test_numeric_settings_reject_toml_booleans", "[settings-store-numeric-type]") {
  QTemporaryDir tempDir;
  REQUIRE(tempDir.isValid());
  QFile toml(tempDir.filePath(QStringLiteral("config.toml")));
  REQUIRE(toml.open(QIODevice::WriteOnly));
  REQUIRE(toml.write("schema_version = 1\n\n[settings]\ndeskew_deviation_coef = true\n") > 0);
  toml.close();

  QString error;
  CHECK_FALSE(SettingsStore::initialize(tempDir.path(), true, &error));
  CHECK(error.contains(QStringLiteral("settings/deskew_deviation_coef")));
}

TEST_CASE("SettingsStoreTestSuite / test_sizef_components_reject_toml_booleans", "[settings-store-sizef-type]") {
  QTemporaryDir tempDir;
  REQUIRE(tempDir.isValid());
  QFile toml(tempDir.filePath(QStringLiteral("config.toml")));
  REQUIRE(toml.open(QIODevice::WriteOnly));
  REQUIRE(toml.write("schema_version = 1\n\n[settings]\nmax_logical_thumb_size = [true, 20.0]\n") > 0);
  toml.close();

  QString error;
  CHECK_FALSE(SettingsStore::initialize(tempDir.path(), true, &error));
  CHECK(error.contains(QStringLiteral("settings/max_logical_thumb_size")));
}

TEST_CASE("SettingsStoreTestSuite / test_invalid_window_geometry_falls_back_without_blocking_writes",
          "[settings-store-geometry-fallback]") {
  QTemporaryDir tempDir;
  REQUIRE(tempDir.isValid());
  QFile toml(tempDir.filePath(QStringLiteral("config.toml")));
  REQUIRE(toml.open(QIODevice::WriteOnly));
  REQUIRE(toml.write("schema_version = 1\n\n[mainWindow]\nnonMaximizedGeometry = 42\n") > 0);
  toml.close();

  QString error;
  REQUIRE(SettingsStore::initialize(tempDir.path(), true, &error));
  SettingsStore settings;
  CHECK(settings.byteArrayValue(QStringLiteral("mainWindow/nonMaximizedGeometry")).isEmpty());
  CHECK_FALSE(settings.hasError());
  settings.setValue(QStringLiteral("settings/language"), QStringLiteral("en"));
  CHECK_FALSE(settings.hasError());
}

TEST_CASE("SettingsStoreTestSuite / test_readonly_default_keeps_legacy_settings_in_memory",
          "[settings-store-readonly]") {
#if defined(Q_OS_LINUX)
  QTemporaryDir tempDir;
  REQUIRE(tempDir.isValid());
  qputenv("XDG_CONFIG_HOME", QByteArray("/proc/scantailor-readonly-config-test"));
  QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, tempDir.filePath(QStringLiteral("legacy")));
  const QString applicationName = QString::fromUtf8(APPLICATION_NAME);
  QCoreApplication::setOrganizationName(applicationName);
  QCoreApplication::setApplicationName(applicationName);

  QSettings legacy(QSettings::IniFormat, QSettings::UserScope, applicationName, applicationName);
  legacy.setValue(QStringLiteral("settings/enable_opengl"), true);
  legacy.sync();
  REQUIRE(legacy.status() == QSettings::NoError);
  const QString legacyPath = legacy.fileName();

  QString warning;
  REQUIRE(SettingsStore::initialize(QString(), false, &warning, true));
  CHECK_FALSE(SettingsStore::configDirectoryWritable());
  CHECK_FALSE(warning.isEmpty());
  CHECK_FALSE(QFileInfo::exists(core::toQString(SettingsStore::configFilePath())));
  SettingsStore settings;
  CHECK(settings.boolValue(QStringLiteral("settings/enable_opengl")));
  settings.sync();
  CHECK(settings.boolValue(QStringLiteral("settings/enable_opengl")));
  CHECK_FALSE(QFileInfo::exists(core::toQString(SettingsStore::configFilePath())));
  CHECK(QFileInfo::exists(legacyPath));
#else
  SUCCEED("The read-only default-path fixture uses Linux's /proc filesystem.");
#endif
}
