// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#ifndef SCANTAILOR_CORE_COLORSCHEME_H_
#define SCANTAILOR_CORE_COLORSCHEME_H_

#include <foundation/Hashes.h>

#include <QColor>
#include <unordered_map>

class QStyle;
class QPalette;

class ColorScheme {
 public:
  virtual ~ColorScheme() = default;

  virtual QStyle* getStyle() const = 0;

  virtual const QPalette* getPalette() const = 0;

  virtual const QString* getStyleSheet() const = 0;

  using ColorParams = std::unordered_map<QString, QColor, hashes::hash<QString>>;

  /**
   * List of colors for elements that don't support styling.
   *
   * @return the list of colors to override the default values by the color scheme
   */
  virtual const ColorParams* getColorParams() const = 0;
};

#endif  // SCANTAILOR_CORE_COLORSCHEME_H_
