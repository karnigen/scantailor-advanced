// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#include <SmartFilenameOrdering.h>

#include <QFileInfo>
#include <QString>
#include <catch2/catch_test_macros.hpp>

namespace Tests {

TEST_CASE("SmartFilenameOrderingTestSuite / test_same_file") {
  const SmartFilenameOrdering less;
  const QFileInfo somefile("/etc/somefile");
  CHECK(!less(somefile, somefile));
}

TEST_CASE("SmartFilenameOrderingTestSuite / test_dirs_different") {
  const SmartFilenameOrdering less;
  const QFileInfo lhs("/etc/file");
  const QFileInfo rhs("/ect/file");
  CHECK(less(lhs, rhs) == (lhs.absolutePath() < rhs.absolutePath()));
  CHECK(less(rhs, lhs) == (rhs.absolutePath() < lhs.absolutePath()));
}

TEST_CASE("SmartFilenameOrderingTestSuite / test_simple_case") {
  const SmartFilenameOrdering less;
  const QFileInfo lhs("/etc/1.png");
  const QFileInfo rhs("/etc/2.png");
  CHECK(less(lhs, rhs));
  CHECK(!less(rhs, lhs));
}

TEST_CASE("SmartFilenameOrderingTestSuite / test_avg_case") {
  const SmartFilenameOrdering less;
  const QFileInfo lhs("/etc/a_0002.png");
  const QFileInfo rhs("/etc/a_1.png");
  CHECK(!less(lhs, rhs));
  CHECK(less(rhs, lhs));
}

TEST_CASE("SmartFilenameOrderingTestSuite / test_compex_case") {
  const SmartFilenameOrdering less;
  const QFileInfo lhs("/etc/a10_10.png");
  const QFileInfo rhs("/etc/a010_2.png");
  CHECK(!less(lhs, rhs));
  CHECK(less(rhs, lhs));
}

TEST_CASE("SmartFilenameOrderingTestSuite / test_almost_equal") {
  const SmartFilenameOrdering less;
  const QFileInfo lhs("/etc/10.png");
  const QFileInfo rhs("/etc/010.png");
  CHECK(!less(lhs, rhs));
  CHECK(less(rhs, lhs));
}

}  // namespace Tests