// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include <BWColor.h>
#include <BinaryImage.h>
#include <Shear.h>

#include <QImage>
#include <catch2/catch_test_macros.hpp>

#include "Utils.h"

namespace imageproc {
namespace tests {
using namespace utils;


TEST_CASE("ShearTestSuite / test_small_image") {
  static const int inp[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 1, 0,
                            0, 1, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 1, 0,
                            0, 1, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

  static const int h_out[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1, 1, 1,
                              0, 0, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0,
                              1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

  static const int v_out[] = {0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0,
                              0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0,
                              0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0};

  const BinaryImage img(makeBinaryImage(inp, 9, 9));
  const BinaryImage hOutImg(makeBinaryImage(h_out, 9, 9));
  const BinaryImage vOutImg(makeBinaryImage(v_out, 9, 9));

  const BinaryImage hShear = imageproc::hShear(img, -1.0, 0.5 * img.height(), WHITE);
  REQUIRE(hShear == hOutImg);

  const BinaryImage vShear = imageproc::vShear(img, 1.0, 0.5 * img.width(), WHITE);
  REQUIRE(vShear == vOutImg);

  BinaryImage hShearInplace(img);
  hShearInPlace(hShearInplace, -1.0, 0.5 * img.height(), WHITE);
  REQUIRE(hShearInplace == hOutImg);

  BinaryImage vShearInplace(img);
  vShearInPlace(vShearInplace, 1.0, 0.5 * img.width(), WHITE);
  REQUIRE(vShearInplace == vOutImg);
}

}  // namespace tests
}  // namespace imageproc