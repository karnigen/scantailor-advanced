// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#include "ContentSpanFinder.h"

#include <SlicedHistogram.h>

using namespace imageproc;

void ContentSpanFinder::findImpl(const SlicedHistogram& histogram,
                                 const VirtualFunction<void, const Span&>& handler) const {
  const auto histSize = static_cast<int>(histogram.size());

  int i = 0;
  int contentEnd = -m_minWhitespaceWidth;
  int contentBegin = contentEnd;

  while (true) {
    // Find the next content position.
    for (; i < histSize; ++i) {
      if (histogram[i] != 0) {
        break;
      }
    }

    if (i - contentEnd >= m_minWhitespaceWidth) {
      // Whitespace is long enough to break the content block.

      // Note that contentEnd is initialized to
      // -m_minWhitespaceWidth to make this test
      // pass on the first content block, in order to avoid
      // growing a non existing content block.

      if (contentEnd - contentBegin >= m_minContentWidth) {
        handler(Span(contentBegin, contentEnd));
      }

      contentBegin = i;
    }

    if (i == histSize) {
      break;
    }

    // Find the next whitespace position.
    for (; i < histSize; ++i) {
      if (histogram[i] == 0) {
        break;
      }
    }
    contentEnd = i;
  }

  if (contentEnd - contentBegin >= m_minContentWidth) {
    handler(Span(contentBegin, contentEnd));
  }
}  // ContentSpanFinder::findImpl
