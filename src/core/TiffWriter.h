// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#ifndef SCANTAILOR_CORE_TIFFWRITER_H_
#define SCANTAILOR_CORE_TIFFWRITER_H_

#include <tiff.h>

#include <cstddef>
#include <cstdint>

class QIODevice;
class QString;
class QImage;
class Dpm;

class TiffWriter {
 public:
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


#endif  // ifndef SCANTAILOR_CORE_TIFFWRITER_H_
