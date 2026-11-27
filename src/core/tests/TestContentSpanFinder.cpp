// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#include <ContentSpanFinder.h>
#include <SlicedHistogram.h>
#include <Span.h>

#include <catch2/catch_test_macros.hpp>
#include <vector>

namespace Tests {
using namespace imageproc;


TEST_CASE("ContentSpanFinderTestSuite / test_empty_input") {
  ContentSpanFinder spanFinder;

  std::vector<Span> spans;
  spanFinder.find(SlicedHistogram(), [&](const Span& span) { spans.push_back(span); });

  CHECK(spans.empty());
}

TEST_CASE("ContentSpanFinderTestSuite / test_min_content_width") {
  SlicedHistogram hist;
  hist.setSize(9);
  hist[0] = 0;
  hist[1] = 1;
  hist[2] = 0;
  hist[3] = 1;
  hist[4] = 1;
  hist[5] = 1;
  hist[6] = 0;
  hist[7] = 1;
  hist[8] = 1;

  ContentSpanFinder spanFinder;
  spanFinder.setMinContentWidth(2);

  std::vector<Span> spans;
  spanFinder.find(hist, [&](const Span& span) { spans.push_back(span); });

  REQUIRE(spans.size() == 2);
  REQUIRE((spans[0] == Span(3, 3 + 3)));
  REQUIRE((spans[1] == Span(7, 7 + 2)));
}

TEST_CASE("ContentSpanFinderTestSuite / test_min_whitespace_width") {
  SlicedHistogram hist;
  hist.setSize(9);
  hist[0] = 0;
  hist[1] = 1;
  hist[2] = 0;
  hist[3] = 1;
  hist[4] = 1;
  hist[5] = 0;
  hist[6] = 0;
  hist[7] = 1;
  hist[8] = 1;

  ContentSpanFinder spanFinder;
  spanFinder.setMinWhitespaceWidth(2);

  std::vector<Span> spans;
  spanFinder.find(hist, [&](const Span& span) { spans.push_back(span); });

  REQUIRE(spans.size() == 2);
  REQUIRE((spans[0] == Span(1, 1 + 4)));
  REQUIRE((spans[1] == Span(7, 7 + 2)));
}

TEST_CASE("ContentSpanFinderTestSuite / test_min_content_and_whitespace_width") {
  SlicedHistogram hist;
  hist.setSize(9);
  hist[0] = 0;
  hist[1] = 1;
  hist[2] = 0;
  hist[3] = 1;
  hist[4] = 1;
  hist[5] = 0;
  hist[6] = 0;
  hist[7] = 1;
  hist[8] = 0;

  ContentSpanFinder spanFinder;
  spanFinder.setMinContentWidth(2);
  spanFinder.setMinWhitespaceWidth(2);

  std::vector<Span> spans;
  spanFinder.find(hist, [&](const Span& span) { spans.push_back(span); });

  // Note that although a content block at index 1 is too short,
  // it's still allowed to merge with content at positions 3 and 4
  // because the whitespace between them is too short as well.

  REQUIRE(spans.size() == 1);
  REQUIRE((spans[0] == Span(1, 1 + 4)));
}

}  // namespace Tests