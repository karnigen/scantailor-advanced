// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <foundation/Hashes.h>
#include <foundation/NonCopyable.h>

#include <QString>
#include <functional>
#include <memory>
#include <unordered_map>

class ColorScheme;

class ColorSchemeFactory {
  DECLARE_NON_COPYABLE(ColorSchemeFactory)
 public:
  ColorSchemeFactory();

  using ColorSchemeConstructor = std::function<std::unique_ptr<ColorScheme>()>;

  void registerColorScheme(const QString& scheme, const ColorSchemeConstructor& constructor);

  std::unique_ptr<ColorScheme> create(const QString& scheme) const;

 private:
  using Registry = std::unordered_map<QString, ColorSchemeConstructor, hashes::hash<QString>>;

  Registry m_registry;
};
