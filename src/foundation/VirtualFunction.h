// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <utility>

template <typename Res, typename... ArgTypes>
class VirtualFunction {
 public:
  virtual ~VirtualFunction() {}

  virtual Res operator()(ArgTypes... args) const = 0;
};


template <typename Delegate, typename Res, typename... ArgTypes>
class ProxyFunction : public VirtualFunction<Res, ArgTypes...> {
 public:
  explicit ProxyFunction(Delegate delegate) : m_delegate(delegate) {}

  Res operator()(ArgTypes... args) const override { return m_delegate(args...); }

 private:
  Delegate m_delegate;
};
