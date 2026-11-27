// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <filesystem>

class RelinkablePath;

class AbstractRelinker {
 public:
  virtual ~AbstractRelinker() = default;

  /**
   * Returns the path to be used instead of the given path.
   * The same path will be returned if no substitution is to be made.
   */
  virtual std::filesystem::path substitutionPathFor(const RelinkablePath& origPath) const = 0;
};
