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
  CHECK(SettingsStore::configFilePath() == QDir(expectedDirectory).filePath(QStringLiteral("config.ini")));
  CHECK(SettingsStore::profilesDirectory() == QDir(expectedDirectory).filePath(QStringLiteral("profiles")));
  CHECK(SettingsStore::configDirectoryExplicit());
  CHECK(SettingsStore::configDirectoryWritable());

  SettingsStore settings;
  settings.setValue(QStringLiteral("settings/enabled"), true);
  CHECK(QFileInfo::exists(SettingsStore::configFilePath()));

  DefaultParamsProfileManager profileManager;
  const auto profile = profileManager.createDefaultProfile();
  REQUIRE(profileManager.writeProfile(*profile, QStringLiteral("test-profile")));
  CHECK(QFileInfo::exists(QDir(SettingsStore::profilesDirectory()).filePath(QStringLiteral("test-profile.stp"))));
}

TEST_CASE("SettingsStoreTestSuite / test_copy_existing_standard_ini_to_config_ini", "[settings-store-legacy]") {
  QTemporaryDir tempDir;
  REQUIRE(tempDir.isValid());

  const QString configRoot = QDir(tempDir.path()).filePath(QStringLiteral("platform-config"));
  qputenv("XDG_CONFIG_HOME", configRoot.toLocal8Bit());
  QSettings::setPath(QSettings::IniFormat, QSettings::UserScope,
                     QDir(tempDir.path()).filePath(QStringLiteral("legacy-settings")));

  const QString applicationName = QString::fromUtf8(APPLICATION_NAME);
  QCoreApplication::setOrganizationName(applicationName);
  QCoreApplication::setApplicationName(applicationName);
  QSettings legacySettings(QSettings::IniFormat, QSettings::UserScope, applicationName, applicationName);
  legacySettings.setValue(QStringLiteral("settings/enabled"), true);
  legacySettings.setValue(QStringLiteral("window/geometry"), QByteArray::fromHex("0a0b0c"));
  legacySettings.sync();
  REQUIRE(legacySettings.status() == QSettings::NoError);
  const QString legacyFilePath = legacySettings.fileName();

  const QString defaultDirectory = QDir(configRoot).filePath(applicationName);
  CHECK(SettingsStore::defaultConfigDirectory() == defaultDirectory);

  QString error;
  REQUIRE(SettingsStore::initialize(QString(), false, &error, true));
  REQUIRE(error.isEmpty());
  CHECK(SettingsStore::configFilePath() == QDir(defaultDirectory).filePath(QStringLiteral("config.ini")));
  CHECK(QFileInfo::exists(legacyFilePath));
  CHECK(QFileInfo::exists(SettingsStore::configFilePath()));

  SettingsStore settings;
  CHECK(settings.boolValue(QStringLiteral("settings/enabled")));
  CHECK(settings.byteArrayValue(QStringLiteral("window/geometry")) == QByteArray::fromHex("0a0b0c"));
}
