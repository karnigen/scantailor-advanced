// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#include "ColorSchemeFactory.h"

#include "ColorScheme.h"
#include "DarkColorScheme.h"
#include "LightColorScheme.h"
#include "NativeColorScheme.h"

ColorSchemeFactory::ColorSchemeFactory() {
  registerColorScheme("native", []() { return std::make_unique<NativeColorScheme>(); });
  registerColorScheme("light", []() { return std::make_unique<LightColorScheme>(); });
  registerColorScheme("dark", []() { return std::make_unique<DarkColorScheme>(); });
}

void ColorSchemeFactory::registerColorScheme(const QString& scheme, const ColorSchemeConstructor& constructor) {
  m_registry[scheme] = constructor;
}

std::unique_ptr<ColorScheme> ColorSchemeFactory::create(const QString& scheme) const {
  return m_registry.at(scheme)();
}
