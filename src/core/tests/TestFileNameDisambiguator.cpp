// Copyright (C) 2026  ScanTailor Advanced contributors
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include <FileNameDisambiguator.h>

#include <QDomDocument>

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(FileNameDisambiguatorTestSuite)

BOOST_AUTO_TEST_CASE(test_labels_and_xml_order) {
  FileNameDisambiguator disambiguator;
  const QString firstPath = QStringLiteral("/first/page.tif");
  const QString secondPath = QStringLiteral("/second/page.tif");

  BOOST_CHECK(disambiguator.registerFile(firstPath) == 0);
  BOOST_CHECK(disambiguator.registerFile(secondPath) == 1);
  BOOST_CHECK(disambiguator.registerFile(firstPath) == 0);
  BOOST_CHECK(disambiguator.getLabel(firstPath) == 0);
  BOOST_CHECK(disambiguator.getLabel(secondPath) == 1);

  QDomDocument doc;
  const QDomElement xml = disambiguator.toXml(doc, QStringLiteral("disambiguator"));
  QDomNode node = xml.firstChild();
  BOOST_REQUIRE(node.isElement());
  BOOST_CHECK(node.toElement().attribute(QStringLiteral("file")) == firstPath);
  BOOST_CHECK(node.toElement().attribute(QStringLiteral("label")) == QStringLiteral("0"));

  node = node.nextSibling();
  BOOST_REQUIRE(node.isElement());
  BOOST_CHECK(node.toElement().attribute(QStringLiteral("file")) == secondPath);
  BOOST_CHECK(node.toElement().attribute(QStringLiteral("label")) == QStringLiteral("1"));
  BOOST_CHECK(node.nextSibling().isNull());
}

BOOST_AUTO_TEST_SUITE_END()
