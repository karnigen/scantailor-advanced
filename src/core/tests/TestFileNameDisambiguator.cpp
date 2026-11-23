// Copyright (C) 2026  ScanTailor Advanced contributors
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include <FileNameDisambiguator.h>

#include <QDomDocument>

#include <catch2/catch_test_macros.hpp>


TEST_CASE("FileNameDisambiguatorTestSuite / test_labels_and_xml_order") {
  FileNameDisambiguator disambiguator;
  const QString firstPath = QStringLiteral("/first/page.tif");
  const QString secondPath = QStringLiteral("/second/page.tif");

  CHECK(disambiguator.registerFile(firstPath) == 0);
  CHECK(disambiguator.registerFile(secondPath) == 1);
  CHECK(disambiguator.registerFile(firstPath) == 0);
  CHECK(disambiguator.getLabel(firstPath) == 0);
  CHECK(disambiguator.getLabel(secondPath) == 1);

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
