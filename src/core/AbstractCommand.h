// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#ifndef SCANTAILOR_CORE_ABSTRACTCOMMAND_H_
#define SCANTAILOR_CORE_ABSTRACTCOMMAND_H_

#include <memory>

template <typename Res, typename... ArgTypes>
class AbstractCommand {
 public:
  using Ptr = std::shared_ptr<AbstractCommand>;

  virtual ~AbstractCommand() = default;

  virtual Res operator()(ArgTypes... args) = 0;
};

#endif  // ifndef SCANTAILOR_CORE_ABSTRACTCOMMAND_H_
