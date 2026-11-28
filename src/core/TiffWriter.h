// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <tiff.h>

#include <QString>
#include <cstddef>
#include <cstdint>

class QIODevice;
class QString;
class QImage;
class Dpm;

class TiffWriter {
 public:
  enum class RenderedPageColorMode { Bitonal, Grayscale, IndexedColor, FullColor, Unsupported, Error };

  struct RenderedPageOptimizationResult {
    RenderedPageColorMode mode;
    QString errorMessage;
  };

  /**
   * \brief Writes a QImage in TIFF format to a file.
   *
   * \param filePath The full path to the file.
   * \param image The image to write.  Writing a null image will fail.
   * \return True on success, false on failure.
   */
  static bool writeImage(const QString& filePath, const QImage& image);

  /**
   * \brief Writes a QImage in TIFF format to an IO device.
   *
   * \param device The device to write to.  This device must be
   *        opened for writing and seekable.
   * \param image The image to write.  Writing a null image will fail.
   * \return True on success, false on failure.
   */
  static bool writeImage(QIODevice& device, const QImage& image);

  /**
   * \brief Losslessly reduces a rendered RGB TIFF's color depth when possible.
   *
   * Rewrites black-and-white pages as 1-bit TIFF, grayscale pages as 8-bit TIFF,
   * and pages with at most 256 colors as indexed TIFF. Full-color pages are left
   * unchanged. Rewritten pages use LZW compression and retain their resolution.
   */
  static RenderedPageOptimizationResult optimizeRenderedPdfPage(const QString& filePath, int fallbackDpi);

 private:
  class TiffHandle;

  static void setDpm(const TiffHandle& tif, const Dpm& dpm);

  static bool writeBitonalOrIndexed8Image(const TiffHandle& tif, const QImage& image);

  static bool writeRGB32Image(const TiffHandle& tif, const QImage& image);

  static bool writeARGB32Image(const TiffHandle& tif, const QImage& image);

  static bool write8bitLines(const TiffHandle& tif, const QImage& image);

  static bool writeBinaryLinesAsIs(const TiffHandle& tif, const QImage& image);

  static bool writeBinaryLinesReversed(const TiffHandle& tif, const QImage& image);

  static const uint8_t m_reverseBitsLUT[256];
};
