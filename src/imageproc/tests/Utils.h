// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#ifndef SCANTAILOR_TESTS_UTILS_H_
#define SCANTAILOR_TESTS_UTILS_H_

class QImage;
class QRect;

namespace imageproc {
class BinaryImage;

namespace tests {
namespace utils {
BinaryImage randomBinaryImage(int width, int height);

QImage randomMonoQImage(int width, int height);

QImage randomGrayImage(int width, int height);

BinaryImage makeBinaryImage(const int* data, int width, int height);

QImage makeMonoQImage(const int* data, int width, int height);

QImage makeGrayImage(const int* data, int width, int height);

void dumpBinaryImage(const BinaryImage& img, const char* name = nullptr);

void dumpGrayImage(const QImage& img, const char* name = nullptr);

bool surroundingsIntact(const QImage& img1, const QImage& img2, const QRect& rect);
}  // namespace utils
}  // namespace tests
}  // namespace imageproc
#endif  // ifndef SCANTAILOR_TESTS_UTILS_H_
