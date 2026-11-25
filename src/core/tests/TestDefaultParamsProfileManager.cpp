// Copyright (C) 2026  ScanTailor Advanced contributors
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include <core/DefaultParams.h>
#include <core/DefaultParamsProfileManager.h>

#include <QDomDocument>
#include <QFile>
#include <QTemporaryDir>
#include <catch2/catch_test_macros.hpp>

#include "version.h"

namespace {

QByteArray makeProfile(const QString& version) {
  QDomDocument doc;
  QDomElement profileElement(doc.createElement("profile"));
  doc.appendChild(profileElement);
  profileElement.setAttribute("version", version);
  profileElement.appendChild(DefaultParams().toXml(doc, "default-params"));
  return doc.toByteArray();
}

bool writeProfile(const QString& path, const QString& version) {
  QFile file(path);
  if (!file.open(QIODevice::WriteOnly)) {
    return false;
  }
  const QByteArray contents(makeProfile(version));
  return file.write(contents) == contents.size();
}

}  // namespace

TEST_CASE("DefaultParamsProfileManagerTestSuite / reads_legacy_and_current_profiles") {
  QTemporaryDir tempDir;
  REQUIRE(tempDir.isValid());
  REQUIRE(writeProfile(tempDir.filePath("legacy.stp"), "3"));
  REQUIRE(writeProfile(tempDir.filePath("current.stp"), QString::number(PROJECT_VERSION)));

  const DefaultParamsProfileManager manager(tempDir.path());
  for (const QString& name : {QStringLiteral("legacy"), QStringLiteral("current")}) {
    DefaultParamsProfileManager::LoadStatus status = DefaultParamsProfileManager::IO_ERROR;
    const std::unique_ptr<DefaultParams> profile(manager.readProfile(name, &status));
    REQUIRE(profile);
    CHECK(status == DefaultParamsProfileManager::SUCCESS);
  }
}

TEST_CASE("DefaultParamsProfileManagerTestSuite / rejects_unsupported_profile_versions") {
  QTemporaryDir tempDir;
  REQUIRE(tempDir.isValid());
  REQUIRE(writeProfile(tempDir.filePath("old.stp"), "2"));
  REQUIRE(writeProfile(tempDir.filePath("newer.stp"), QString::number(PROJECT_VERSION + 1)));
  REQUIRE(writeProfile(tempDir.filePath("nonnumeric.stp"), "invalid"));

  const DefaultParamsProfileManager manager(tempDir.path());
  for (const QString& name : {QStringLiteral("old"), QStringLiteral("newer"), QStringLiteral("nonnumeric")}) {
    DefaultParamsProfileManager::LoadStatus status = DefaultParamsProfileManager::SUCCESS;
    const std::unique_ptr<DefaultParams> profile(manager.readProfile(name, &status));
    CHECK_FALSE(profile);
    CHECK(status == DefaultParamsProfileManager::INCOMPATIBLE_VERSION_ERROR);
  }
}
