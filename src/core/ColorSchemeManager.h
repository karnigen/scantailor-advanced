// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#ifndef SCANTAILOR_CORE_COLORSCHEMEMANAGER_H_
#define SCANTAILOR_CORE_COLORSCHEMEMANAGER_H_

#include <foundation/NonCopyable.h>

#include <QBrush>
#include <QColor>
#include <memory>

#include "ColorScheme.h"

class ColorSchemeManager {
  DECLARE_NON_COPYABLE(ColorSchemeManager)
 private:
  ColorSchemeManager() = default;

 public:
  static ColorSchemeManager& instance();

  void setColorScheme(const ColorScheme& colorScheme);

  QBrush getColorParam(const QString& colorParam, const QBrush& defaultBrush) const;

  QColor getColorParam(const QString& colorParam, const QColor& defaultColor) const;

 private:
  std::unique_ptr<ColorScheme::ColorParams> m_colorParams;
};


#endif  // SCANTAILOR_CORE_COLORSCHEMEMANAGER_H_
