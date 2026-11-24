// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <catch2/catch_test_macros.hpp>

#include "SettingsStore.h"

TEST_CASE("SettingsStoreTestSuite / test_values_round_trip") {
  QTemporaryDir tempDir;
  REQUIRE(tempDir.isValid());
  const QString filePath = tempDir.filePath("config.toml");

  {
    SettingsStore settings(filePath);
    settings.setValue("settings/enabled", true);
    settings.setValue("settings/limit", 256);
    settings.setValue("settings/ratio", 1.25);
    settings.setValue("settings/name", QStringLiteral("sample"));
    settings.setValue("window/geometry", QByteArray::fromHex("010203"));
    settings.setValue("window/size", QSize(800, 600));
    settings.setValue("window/logicalSize", QSizeF(400.5, 300.25));
  }

  SettingsStore settings(filePath);
  CHECK(settings.boolValue("settings/enabled"));
  CHECK(settings.intValue("settings/limit") == 256);
  CHECK(settings.doubleValue("settings/ratio") == 1.25);
  CHECK(settings.stringValue("settings/name") == QStringLiteral("sample"));
  CHECK(settings.byteArrayValue("window/geometry") == QByteArray::fromHex("010203"));
  CHECK(settings.sizeValue("window/size") == QSize(800, 600));
  CHECK(settings.sizeFValue("window/logicalSize") == QSizeF(400.5, 300.25));

  settings.setValue("settings/invalidLimit", QStringLiteral("not-an-integer"));
  bool conversionSucceeded = true;
  CHECK(settings.intValue("settings/invalidLimit", 0, &conversionSucceeded) == 0);
  CHECK_FALSE(conversionSucceeded);
  CHECK(settings.hasError());
}

TEST_CASE("SettingsStoreTestSuite / test_string_lists_round_trip_as_toml_arrays") {
  QTemporaryDir tempDir;
  REQUIRE(tempDir.isValid());
  const QString filePath = tempDir.filePath("config.toml");

  {
    SettingsStore settings(filePath);
    settings.setStringList("project/recent", {"/scans/first.ScanTailor", "/scans/second.ScanTailor"});
    settings.setStringList("project/recent", {"/scans/third.ScanTailor"});
  }

  CHECK(SettingsStore(filePath).stringList("project/recent") == QStringList{"/scans/third.ScanTailor"});
}

TEST_CASE("SettingsStoreTestSuite / test_persistence_error_clears_after_successful_retry",
          "[settings-store-persistence-error]") {
  QTemporaryDir tempDir;
  REQUIRE(tempDir.isValid());
  const QString blockedDirectory = tempDir.filePath(QStringLiteral("not-a-directory"));
  QFile blocker(blockedDirectory);
  REQUIRE(blocker.open(QIODevice::WriteOnly));
  blocker.close();

  const QString filePath = QDir(blockedDirectory).filePath(QStringLiteral("config.toml"));
  SettingsStore settings(filePath);
  settings.setValue(QStringLiteral("settings/first"), true);
  CHECK(settings.hasError());

  REQUIRE(QFile::remove(blockedDirectory));
  REQUIRE(QDir().mkpath(blockedDirectory));
  settings.setValue(QStringLiteral("settings/second"), true);
  CHECK_FALSE(settings.hasError());

  QFile savedFile(filePath);
  REQUIRE(savedFile.open(QIODevice::ReadOnly));
  const QByteArray contents = savedFile.readAll();
  CHECK(contents.contains("first = true"));
  CHECK(contents.contains("second = true"));
}
