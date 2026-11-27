// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#ifndef SCANTAILOR_IMAGEPROC_DPM_H_
#define SCANTAILOR_IMAGEPROC_DPM_H_

#include <QSize>

class Dpi;
class QImage;

/**
 * \brief Dots per meter (horizontal and vertical).
 */
class Dpm {
  // Member-wise copying is OK.
 public:
  Dpm() : m_xDpm(0), m_yDpm(0) {}

  Dpm(int horizontal, int vertical) : m_xDpm(horizontal), m_yDpm(vertical) {}

  Dpm(Dpi dpi);

  explicit Dpm(QSize size);

  explicit Dpm(const QImage& image);

  int horizontal() const { return m_xDpm; }

  int vertical() const { return m_yDpm; }

  QSize toSize() const;

  bool isNull() const;

  bool operator==(const Dpm& other) const;

  bool operator!=(const Dpm& other) const { return !(*this == other); }

 private:
  int m_xDpm;
  int m_yDpm;
};


#endif  // ifndef SCANTAILOR_IMAGEPROC_DPM_H_
