// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#ifndef SCANTAILOR_CORE_NATIVECOLORSCHEME_H_
#define SCANTAILOR_CORE_NATIVECOLORSCHEME_H_

#include <QString>

#include "ColorScheme.h"

class NativeColorScheme : public ColorScheme {
 public:
  NativeColorScheme();

  QStyle* getStyle() const override;

  const QPalette* getPalette() const override;

  const QString* getStyleSheet() const override;

  const ColorParams* getColorParams() const override;

 private:
  void loadStyleSheet();

  void loadColorParams();

  QString m_styleSheet;
  ColorParams m_customColors;
};

#endif  // SCANTAILOR_CORE_NATIVECOLORSCHEME_H_
