// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#ifndef SCANTAILOR_CORE_LIGHTCOLORSCHEME_H_
#define SCANTAILOR_CORE_LIGHTCOLORSCHEME_H_

#include <QPalette>
#include <QString>
#include <memory>

#include "ColorScheme.h"

class LightColorScheme : public ColorScheme {
 public:
  LightColorScheme();

  QStyle* getStyle() const override;

  const QPalette* getPalette() const override;

  const QString* getStyleSheet() const override;

  const ColorParams* getColorParams() const override;

 private:
  void loadPalette();

  void loadStyleSheet();

  void loadColorParams();

  QPalette m_palette;
  QString m_styleSheet;
  ColorParams m_customColors;
};


#endif  // SCANTAILOR_CORE_LIGHTCOLORSCHEME_H_
