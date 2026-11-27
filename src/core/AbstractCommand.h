// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <memory>

template <typename Res, typename... ArgTypes>
class AbstractCommand {
 public:
  using Ptr = std::shared_ptr<AbstractCommand>;

  virtual ~AbstractCommand() = default;

  virtual Res operator()(ArgTypes... args) = 0;
};
