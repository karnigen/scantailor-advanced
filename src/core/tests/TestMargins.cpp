// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#include <Margins.h>

#include <QDomDocument>
#include <QDomElement>
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>


TEST_CASE("CoreMarginsTestSuite / test_default_constructor") {
  const Margins m;
  CHECK(m.left() == 0.0);
  CHECK(m.top() == 0.0);
  CHECK(m.right() == 0.0);
  CHECK(m.bottom() == 0.0);
}

TEST_CASE("CoreMarginsTestSuite / test_value_constructor") {
  const Margins m(1.0, 2.0, 3.0, 4.0);
  CHECK(m.left() == 1.0);
  CHECK(m.top() == 2.0);
  CHECK(m.right() == 3.0);
  CHECK(m.bottom() == 4.0);
}

TEST_CASE("CoreMarginsTestSuite / test_setters") {
  Margins m;
  m.setLeft(10.0);
  m.setTop(20.0);
  m.setRight(30.0);
  m.setBottom(40.0);
  CHECK(m.left() == 10.0);
  CHECK(m.top() == 20.0);
  CHECK(m.right() == 30.0);
  CHECK(m.bottom() == 40.0);
}

TEST_CASE("CoreMarginsTestSuite / test_xml_roundtrip") {
  const Margins original(1.5, 2.5, 3.5, 4.5);
  QDomDocument doc;
  QDomElement el = original.toXml(doc, "margins");
  doc.appendChild(el);

  const QDomElement readEl = doc.documentElement();
  const Margins fromXml(readEl);
  CHECK((fromXml.left()) == Catch::Approx(original.left()).epsilon((0.01) / 100.0));
  CHECK((fromXml.top()) == Catch::Approx(original.top()).epsilon((0.01) / 100.0));
  CHECK((fromXml.right()) == Catch::Approx(original.right()).epsilon((0.01) / 100.0));
  CHECK((fromXml.bottom()) == Catch::Approx(original.bottom()).epsilon((0.01) / 100.0));
}

TEST_CASE("CoreMarginsTestSuite / test_from_xml_attributes") {
  QDomDocument doc;
  QDomElement el = doc.createElement("margins");
  el.setAttribute("left", "1.0");
  el.setAttribute("top", "2.0");
  el.setAttribute("right", "3.0");
  el.setAttribute("bottom", "4.0");
  const Margins m(el);
  CHECK((m.left()) == Catch::Approx(1.0).epsilon((0.01) / 100.0));
  CHECK((m.top()) == Catch::Approx(2.0).epsilon((0.01) / 100.0));
  CHECK((m.right()) == Catch::Approx(3.0).epsilon((0.01) / 100.0));
  CHECK((m.bottom()) == Catch::Approx(4.0).epsilon((0.01) / 100.0));
}
