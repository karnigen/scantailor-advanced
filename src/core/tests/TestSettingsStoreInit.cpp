// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include <config.h>
#include <core/DefaultParams.h>
#include <core/DefaultParamsProfileManager.h>
#include <core/SettingsStore.h>

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSettings>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <catch2/catch_test_macros.hpp>

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
  CHECK(SettingsStore::configDirectory() == expectedDirectory);
  CHECK(SettingsStore::configFilePath() == QDir(expectedDirectory).filePath(QStringLiteral("config.toml")));
  CHECK(SettingsStore::profilesDirectory() == QDir(expectedDirectory).filePath(QStringLiteral("profiles")));
  CHECK(SettingsStore::configDirectoryExplicit());
  CHECK(SettingsStore::configDirectoryWritable());

  SettingsStore settings;
  settings.setValue(QStringLiteral("settings/enabled"), true);
  CHECK(QFileInfo::exists(SettingsStore::configFilePath()));
  CHECK_FALSE(QFileInfo::exists(QDir(expectedDirectory).filePath(QStringLiteral("config.ini"))));

  DefaultParamsProfileManager profileManager;
  const auto profile = profileManager.createDefaultProfile();
  REQUIRE(profileManager.writeProfile(*profile, QStringLiteral("test-profile")));
  CHECK(QFileInfo::exists(QDir(SettingsStore::profilesDirectory()).filePath(QStringLiteral("test-profile.stp"))));
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

  const QString defaultDirectory = SettingsStore::defaultConfigDirectory();
  const QString relativeDefaultDirectory = QDir(tempDir.path()).relativeFilePath(defaultDirectory);
  CHECK(!relativeDefaultDirectory.startsWith(QStringLiteral("..")));

  QString error;
  REQUIRE(SettingsStore::initialize(QString(), false, &error, true));
  REQUIRE(error.isEmpty());
  CHECK(SettingsStore::configFilePath() == QDir(defaultDirectory).filePath(QStringLiteral("config.toml")));
  CHECK(QFileInfo::exists(legacyFilePath));
  CHECK(QFileInfo::exists(QDir(defaultDirectory).filePath(QStringLiteral("config.ini"))));
  CHECK(QFileInfo::exists(SettingsStore::configFilePath()));

  SettingsStore settings;
  CHECK(settings.boolValue(QStringLiteral("settings/enable_opengl")));
  CHECK(settings.byteArrayValue(QStringLiteral("window/geometry")) == QByteArray::fromHex("0a0b0c"));
}

TEST_CASE("SettingsStoreTestSuite / test_migrate_ini_to_toml_and_keep_source", "[settings-store-migration]") {
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
  CHECK(SettingsStore::configFilePath() == tempDir.filePath(QStringLiteral("config.toml")));
  CHECK(QFileInfo::exists(iniPath));

  SettingsStore settings;
  CHECK(settings.boolValue(QStringLiteral("settings/enable_opengl")));
  CHECK(settings.sizeValue(QStringLiteral("settings/thumbnail_quality")) == QSize(120, 80));
  CHECK(settings.byteArrayValue(QStringLiteral("window/nonMaximizedGeometry")) == geometry);
  CHECK(settings.stringValue(QStringLiteral("custom/unknown")) == QStringLiteral("preserve me"));
  CHECK((settings.stringList(QStringLiteral("project/recent"))
         == QStringList{QStringLiteral("/scans/first.ScanTailor"), QStringLiteral("/scans/second.ScanTailor")}));

  QFile unchangedIni(iniPath);
  REQUIRE(unchangedIni.open(QIODevice::ReadOnly));
  CHECK(unchangedIni.readAll() == originalIni);

  settings.setValue(QStringLiteral("settings/enable_opengl"), false);
  REQUIRE(SettingsStore::initialize(tempDir.path(), true, &error));
  CHECK_FALSE(settings.boolValue(QStringLiteral("settings/enable_opengl")));
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
  CHECK(SettingsStore(tempDir.filePath(QStringLiteral("config.toml"))).stringValue(QStringLiteral("custom/retained"))
        == QStringLiteral("keep"));
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
  REQUIRE(toml.open(QIODevice::WriteOnly | QIODevice::Text));
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
