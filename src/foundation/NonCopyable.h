// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#define DECLARE_NON_COPYABLE(Class)            \
 public:                                       \
  /** \brief Copying is forbidden. */          \
  Class(const Class&) = delete;                \
  /** \brief Copying is forbidden. */          \
  Class& operator=(const Class&) = delete;     \
  /** \brief Moving is forbidden. */           \
  Class(Class&&) noexcept = delete;            \
  /** \brief Moving is forbidden. */           \
  Class& operator=(Class&&) noexcept = delete; \
                                               \
 private:
