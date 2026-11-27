// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#ifndef SCANTAILOR_CORE_PAGEORDEROPTION_H_
#define SCANTAILOR_CORE_PAGEORDEROPTION_H_

#include <QString>
#include <memory>

#include "PageOrderProvider.h"

class PageOrderOption {
  // Member-wise copying is OK.
 public:
  using ProviderPtr = std::shared_ptr<const PageOrderProvider>;

  PageOrderOption(const QString& name, ProviderPtr provider) : m_name(name), m_provider(std::move(provider)) {}

  const QString& name() const { return m_name; }

  /**
   * Returns the ordering information provider.
   * A null provider is OK and is to be interpreted as default order.
   */
  const ProviderPtr& provider() const { return m_provider; }

 private:
  QString m_name;
  ProviderPtr m_provider;
};


#endif  // ifndef SCANTAILOR_CORE_PAGEORDEROPTION_H_
