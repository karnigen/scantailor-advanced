// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#include <BinaryImage.h>
#include <SkewFinder.h>

#include <QApplication>
#include <QColor>
#include <QImage>
#include <QPainter>
#include <QTransform>
#include <catch2/catch_test_macros.hpp>
#include <cmath>
#include <cstdlib>

namespace imageproc {
namespace tests {

TEST_CASE("SkewFinderTestSuite / test_positive_detection") {
  int argc = 1;
  char argv0[] = "test";
  char* argv[1] = {argv0};
  QApplication app(argc, argv);
  QImage image(1000, 800, QImage::Format_ARGB32_Premultiplied);
  image.fill(0xffffffff);
  {
    // Draw parallel strokes in "skewed page" space (no QTextDocument / fonts).
    // Headless RPM/mock chroots often have no fonts; QPainter::drawText() can abort there.
    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing, false);
    painter.setPen(QPen(QColor(0, 0, 0), 2));
    QTransform xform1;
    xform1.translate(-0.5 * image.width(), -0.5 * image.height());
    QTransform xform2;
    xform2.rotate(4.5);
    QTransform xform3;
    xform3.translate(0.5 * image.width(), 0.5 * image.height());
    painter.setWorldTransform(xform1 * xform2 * xform3);
    for (int y = -500; y <= 500; y += 14) {
      painter.drawLine(QLineF(-700, y, 700, y));
    }
  }

  SkewFinder skewFinder;
  const Skew skew(skewFinder.findSkew(BinaryImage(image)));
  REQUIRE(std::fabs(skew.angle() - 4.5) < 0.15);
  CHECK(skew.confidence() >= Skew::GOOD_CONFIDENCE);
}

TEST_CASE("SkewFinderTestSuite / test_negative_detection") {
  QImage image(1000, 800, QImage::Format_Mono);
  image.fill(1);

  const int numDots = image.width() * image.height() / 5;
  for (int i = 0; i < numDots; ++i) {
    const int x = rand() % image.width();
    const int y = rand() % image.height();
    image.setPixel(x, y, 0);
  }

  SkewFinder skewFinder;
  skewFinder.setCoarseReduction(0);
  skewFinder.setFineReduction(0);
  const Skew skew(skewFinder.findSkew(BinaryImage(image)));
  CHECK(skew.confidence() < Skew::GOOD_CONFIDENCE);
}

}  // namespace tests
}  // namespace imageproc