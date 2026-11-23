// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include <Binarize.h>
#include <BinaryImage.h>

#include <QImage>
#include <QSize>
#include <catch2/catch_test_macros.hpp>

#include "Utils.h"

namespace imageproc {
namespace tests {
using namespace utils;

#if 0
            TEST_CASE("BinarizeTestSuite / test") {
                QImage img("test.png");
                binarizeWolf(img).toQImage().save("out.png");
            }
#endif
}  // namespace tests
}  // namespace imageproc
