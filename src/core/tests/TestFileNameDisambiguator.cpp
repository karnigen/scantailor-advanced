// SPDX-FileCopyrightText: 2026 project contributors
// SPDX-License-Identifier: GPL-3.0-only

#include <FileNameDisambiguator.h>
#include <core/PathUtils.h>

#include <QDomDocument>

#include <catch2/catch_test_macros.hpp>


TEST_CASE("FileNameDisambiguatorTestSuite / test_labels_and_xml_order") {
  FileNameDisambiguator disambiguator;
  const QString firstPath = QStringLiteral("/first/page.tif");
  const QString secondPath = QStringLiteral("/second/page.tif");

  const std::filesystem::path firstFilePath(core::toFileSystemPath(firstPath));
  const std::filesystem::path secondFilePath(core::toFileSystemPath(secondPath));
  CHECK(disambiguator.registerFile(firstFilePath) == 0);
  CHECK(disambiguator.registerFile(secondFilePath) == 1);
  CHECK(disambiguator.registerFile(firstFilePath) == 0);
  CHECK(disambiguator.getLabel(firstFilePath) == 0);
  CHECK(disambiguator.getLabel(secondFilePath) == 1);

  QDomDocument doc;
  const QDomElement xml = disambiguator.toXml(doc, QStringLiteral("disambiguator"));
  QDomNode node = xml.firstChild();
  REQUIRE(node.isElement());
  CHECK(node.toElement().attribute(QStringLiteral("file")) == firstPath);
  CHECK(node.toElement().attribute(QStringLiteral("label")) == QStringLiteral("0"));

  node = node.nextSibling();
  REQUIRE(node.isElement());
  CHECK(node.toElement().attribute(QStringLiteral("file")) == secondPath);
  CHECK(node.toElement().attribute(QStringLiteral("label")) == QStringLiteral("1"));
  CHECK(node.nextSibling().isNull());
}
