// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#ifndef SCANTAILOR_CORE_DEFAULTPARAMSPROVIDER_H_
#define SCANTAILOR_CORE_DEFAULTPARAMSPROVIDER_H_

#include <foundation/NonCopyable.h>

#include <QtCore/QString>
#include <memory>

class DefaultParams;

class DefaultParamsProvider {
  DECLARE_NON_COPYABLE(DefaultParamsProvider)
 private:
  DefaultParamsProvider();

 public:
  static DefaultParamsProvider& getInstance();

  const QString& getProfileName() const;

  const DefaultParams& getParams() const;

  void setParams(std::unique_ptr<DefaultParams> params, const QString& name);

 private:
  QString m_profileName;
  std::unique_ptr<DefaultParams> m_params;
};


#endif  // SCANTAILOR_CORE_DEFAULTPARAMSPROVIDER_H_
