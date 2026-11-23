// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include <Dpi.h>
#include <Dpm.h>

#include <QDomDocument>
#include <QDomElement>
#include <QSize>
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>


TEST_CASE("ImageProcDpiTestSuite / test_default_constructor_is_null") {
  const Dpi dpi;
  CHECK(dpi.isNull());
  CHECK((dpi.horizontal() <= 1 || dpi.vertical() <= 1));
}

TEST_CASE("ImageProcDpiTestSuite / test_value_constructor") {
  const Dpi dpi(300, 600);
  CHECK(!dpi.isNull());
  CHECK((dpi.horizontal()) == (300));
  CHECK((dpi.vertical()) == (600));
}

TEST_CASE("ImageProcDpiTestSuite / test_equality") {
  const Dpi a(300, 300);
  const Dpi b(300, 300);
  const Dpi c(300, 600);
  CHECK(a == b);
  CHECK(a != c);
}

TEST_CASE("ImageProcDpiTestSuite / test_to_size") {
  const Dpi dpi(72, 96);
  const QSize size = dpi.toSize();
  CHECK((size.width()) == (72));
  CHECK((size.height()) == (96));
}

TEST_CASE("ImageProcDpiTestSuite / test_to_size_null_returns_empty") {
  const Dpi dpi(0, 0);
  const QSize size = dpi.toSize();
  CHECK(size.isEmpty());
}

TEST_CASE("ImageProcDpiTestSuite / test_from_qsize") {
  const QSize size(150, 200);
  const Dpi dpi(size);
  CHECK((dpi.horizontal()) == (150));
  CHECK((dpi.vertical()) == (200));
}

TEST_CASE("ImageProcDpiTestSuite / test_xml_roundtrip") {
  const Dpi original(300, 600);
  QDomDocument doc;
  QDomElement el = original.toXml(doc, "dpi");
  doc.appendChild(el);

  const QDomElement readEl = doc.documentElement();
  const Dpi fromXml(readEl);
  CHECK((fromXml.horizontal()) == (original.horizontal()));
  CHECK((fromXml.vertical()) == (original.vertical()));
}

TEST_CASE("ImageProcDpiTestSuite / test_from_xml_attributes") {
  QDomDocument doc;
  QDomElement el = doc.createElement("dpi");
  el.setAttribute("horizontal", "200");
  el.setAttribute("vertical", "400");
  const Dpi dpi(el);
  CHECK((dpi.horizontal()) == (200));
  CHECK((dpi.vertical()) == (400));
}

TEST_CASE("ImageProcDpiTestSuite / test_from_dpm") {
  const Dpm dpm(11811, 23622);
  const Dpi dpi(dpm);
  CHECK(dpi.horizontal() > 0);
  CHECK(dpi.vertical() > 0);
  CHECK((static_cast<double>(dpi.horizontal()) / dpm.horizontal()) == Catch::Approx(0.0254).epsilon((0.1) / 100.0));
}
